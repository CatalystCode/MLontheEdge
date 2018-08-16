import glob
import json
import logging
import os
from   datetime import datetime
import time
import uuid
from   logging.handlers import TimedRotatingFileHandler

import iothub_client
from   iothub_client import (DeviceMethodReturnValue, IoTHubClient,
                             IoTHubClientError, IoTHubError, IoTHubMessage,
                             IoTHubMessageDispositionResult,
                             IoTHubTransportProvider)


from   azure.storage.blob import (
       ContentSettings,
       BlockBlobService
)

from   modules.config_manager import config_manager, protocol_error
from   modules.scheduler import scheduler
import threading

# Import main so we can get the root path of the program
import __main__

# HTTP options
# Because it can poll "after 9 seconds" polls will happen effectively
# at ~10 seconds.
# Note that for scalabilty, the default value of minimumPollingTime
# is 25 minutes. For more information, see:
# https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
TIMEOUT = 241000
MINIMUM_POLLING_TIME = 9

# messageTimeout - the maximum time in milliseconds until a message times out.
# The timeout period starts at IoTHubClient.send_event_async.
# By default, messages do not expire.
MESSAGE_TIMEOUT = 10000

# Stolen from this Stack Overflow Answer to help format the milliseconds in the log entry time.
# https://stackoverflow.com/a/6290946
class log_time_formatter(logging.Formatter):
    """
    Used to format the time with milliseconds in the log
    """
    converter=datetime.fromtimestamp
    def formatTime(self, record, datefmt=None):
        ct = self.converter(record.created)
        if datefmt:
            s = ct.strftime(datefmt)
        else:
            t = ct.strftime("%Y-%m-%d %H:%M:%S")
            s = "%s,%03d" % (t, record.msecs)
        return s


class iothub_manager(object):
    def nop_callback(self,*args):
        """
        Non-op callback.  Used as a callback when the response is not important.
        """
        pass

    def set_reported_property(self, prop, val):
        reported_state = json.dumps({str(prop): val})
        self.client.send_reported_state(reported_state, len(reported_state), self.nop_callback, 0)

    def __init__(
            self,
            device_method_callback,
            sensor_callback, 
            image_callback, 
            video_callback
    ):
        """
        Initializes the iothub_manager, but does not start the iot client.  Use iothub_manager.start() to actually start the client
        """

        # This flag is used to keep the schedulers from starting until the device has initialized
        self.schedulers_enabled = False

        self.config_manager = config_manager()
        self.set_variables_from_config()

        self.init_logger()

        self.init_azure_storage()

        self.SCRIPT_DIR = os.path.split(os.path.realpath(__main__.__file__))[0]

        self.sensor_scheduler   = None
        self.image_scheduler    = None
        self.video_scheduler    = None
        self.watchdog_scheduler = None

        self.video_upload_scheduler = None
        self.is_video_uploading = False

        self.log_upload_scheduler = None
        self.is_log_uploading = False

        self.RECEIVE_CONTEXT = 0
        self.TWIN_CONTEXT = 0
        self.METHOD_CONTEXT = 0
        self.last_watchdog_time = 0

        # Set the callbacks for the sensor, image, and video capture schedulers
        self.sensor_callback = sensor_callback
        self.image_callback = image_callback
        self.video_callback = video_callback
        self.device_method_callback = device_method_callback

    def start(self):
        """
        Connects to the IoT Hub
        """

        # Connect to the iot hub
        self.protocol = self.config_manager.get_protocol_provider()
        self.connection_string = self.config_manager.get_device_connection_string()
        self.client = IoTHubClient(self.connection_string, self.protocol)

        # Set the IoT Hub method callbacks
        self.client.set_device_twin_callback(self.device_twin_callback, self.TWIN_CONTEXT)
        self.client.set_message_callback(self.receive_message_callback, self.RECEIVE_CONTEXT)
        self.client.set_device_method_callback(self.device_method_callback, self.METHOD_CONTEXT) 
        
        if self.protocol == IoTHubTransportProvider.HTTP:
            self.client.set_option("timeout", TIMEOUT)
            self.client.set_option("MinimumPollingTime", MINIMUM_POLLING_TIME)
        # set the time until a message times out
        self.client.set_option("messageTimeout", MESSAGE_TIMEOUT)

    def init_logger(self):
        """
        Configures the logger based on the configuration values retrieved from the device twin.
        """
        self.logger = logging.getLogger('GourdDevice')
        self.logger.setLevel(self.LogLevel)

        # Log format:
        # the entry time (asctime) in the format '%Y-%m-%d %H:%M:%S.%f' (see log_time_formatter class def above)
        # The module name in a left aligned 14 character wide column
        # The debug level of the entry as a single character in square brackets
        # The actual message being logged
        formatter = log_time_formatter('%(asctime).23s %(module)-14s [%(levelname).1s] %(message)s',datefmt='%Y-%m-%d %H:%M:%S.%f')

        # reset the handlers
        self.logger.handlers = []

        # Setup console / timed rotating file handler (>DEBUG)
        self.console_handler = logging.StreamHandler()
        self.console_handler.setFormatter(formatter)
        self.logger.addHandler(self.console_handler)

        self.timed_handler = TimedRotatingFileHandler(os.path.dirname(os.path.abspath(__file__)) + '/../logs/gourddevice', when=self.LogWhen, interval=self.LogInterval, backupCount=0, utc=True)
        self.timed_handler.suffix = '%Y-%m-%d_%H-%M-%S.log'
        self.timed_handler.setFormatter(formatter)
        self.logger.addHandler(self.timed_handler)
    
    def init_azure_storage(self):
        """
        Configures the Azure Blob Service Client based on the values received from the device twin
        """
        self.blob_service = None
        if(((self.StorageAccountName != "") and (self.StorageAccountName != ""))):
            self.StorageAccountConnectionString = "AccountName={0};AccountKey={1};".format(self.StorageAccountName, self.StorageAccountKey)
            self.blob_service = BlockBlobService(connection_string = self.StorageAccountConnectionString)
        else:
            self.logger.error("The StorageAccountName, StorageAccountKey, and/or StorageAccountCounter are not set correctly in the device twin")

    def set_variables_from_config(self):
        """
        Loads values received from the device twin into variables for easy reference.  Assigns appropriate defaults to missing config values.
        """
        # TODO: Need to clean this up / Add null/safety checks
        self.LogLevel                       = logging.getLevelName(self.config_manager.get_config_value("Twin|desired|LogLevel","DEBUG").upper().rstrip())
        if not isinstance(self.LogLevel, int):   # Convert level enum to corresponding int
            self.LogLevel    = logging.getLevelName('DEBUG')
        self.LogWhen                        = self.config_manager.get_config_value("Twin|desired|LogWhen","H").lower().rstrip()
        self.LogInterval                    = self.config_manager.get_config_value("Twin|desired|LogInterval",1)

        self.DeviceId                       = self.config_manager.get_config_value("DeviceId","unknown_device")
        self.SensorCaptureFrequency         = self.config_manager.get_config_value("Twin|desired|SensorCaptureFrequency",60)
        self.SensorCaptureStartInterval     = self.config_manager.get_config_value("Twin|desired|SensorCaptureStartInterval",self.SensorCaptureFrequency)
        self.SensorPublishBatchSize         = self.config_manager.get_config_value("Twin|desired|SensorPublishBatchSize",1)
        self.ImageCaptureFrequency          = self.config_manager.get_config_value("Twin|desired|ImageCaptureFrequency",0)
        self.ImageCaptureStartInterval      = self.config_manager.get_config_value("Twin|desired|ImageCaptureStartInterval",self.ImageCaptureFrequency)
        self.ImagePublishBatchSize          = self.config_manager.get_config_value("Twin|desired|ImagePublishBatchSize",1)
        self.VideoCaptureFrequency          = self.config_manager.get_config_value("Twin|desired|VideoCaptureFrequency",0)
        self.VideoCaptureStartInterval      = self.config_manager.get_config_value("Twin|desired|VideoCaptureStartInterval",self.VideoCaptureFrequency)
        self.VideoCaptureDuration           = self.config_manager.get_config_value("Twin|desired|VideoCaptureDuration",0)
        self.VideoPublishBatchSize          = self.config_manager.get_config_value("Twin|desired|VideoPublishBatchSize",1)
        self.TripDistanceThreshold          = self.config_manager.get_config_value("Twin|desired|TripDistanceThreshold",50)
        self.TripSamplePeriod               = self.config_manager.get_config_value("Twin|desired|TripSamplePeriod",0.1)
        self.HouseId                        = self.config_manager.get_config_value("Twin|desired|HouseId","unknown_house")
        self.NestId                         = self.config_manager.get_config_value("Twin|desired|NestId","unknown_nest")
        self.DeviceType                     = self.config_manager.get_config_value("Twin|desired|DeviceType","unknown_type")
        self.ELLEnabled                     = self.config_manager.get_config_value("Twin|desired|ELLEnabled",0)
        self.ELLPredictionInterval          = self.config_manager.get_config_value("Twin|desired|ELLPredictionInterval",5)
        self.ELLTriggerVideoUpload          = self.config_manager.get_config_value("Twin|desired|ELLTriggerVideoUpload",0)
        self.ELLVideoPrerollDuration        = self.config_manager.get_config_value("Twin|desired|ELLVideoPrerollDuration",0)
        self.CameraTemperatureThreshold     = self.config_manager.get_config_value("Twin|desired|CameraTemperatureThreshold",0)
        self.CameraResolutionWidth          = self.config_manager.get_config_value("Twin|desired|CameraResolutionWidth",1280)
        self.CameraResolutionHeight         = self.config_manager.get_config_value("Twin|desired|CameraResolutionHeight",720)
        self.VideoCaptureFrameRate          = self.config_manager.get_config_value("Twin|desired|VideoCaptureFrameRate",24)
        self.VideoUploadSchedulerFrequency  = self.config_manager.get_config_value("Twin|desired|VideoUploadSchedulerFrequency",5)
        self.VideoUploadStartInterval       = self.config_manager.get_config_value("Twin|desired|VideoUploadStartInterval",self.VideoUploadSchedulerFrequency)
        self.LogUploadSchedulerFrequency    = self.config_manager.get_config_value("Twin|desired|LogUploadSchedulerFrequency",5)
        self.LogUploadStartInterval         = self.config_manager.get_config_value("Twin|desired|LogUploadStartInterval",self.LogUploadSchedulerFrequency)
        self.WatchdogMinRebootTime          = self.config_manager.get_config_value("Twin|desired|WatchdogMinRebootTime",900)
        self.WatchdogFrequency              = self.config_manager.get_config_value("Twin|desired|WatchdogFrequency",15)
        self.WatchdogStartInterval          = self.config_manager.get_config_value("Twin|desired|WatchdogStartInterval",self.WatchdogFrequency)

        self.IoTHubEndpoint                 = self.config_manager.get_config_value("IoTHubEndpoint","")
        self.IoTHubProtocol                 = self.config_manager.get_config_value("Protocol","")

        self.StorageAccountName             = self.config_manager.get_config_value("Twin|desired|StorageAccountName","")
        self.StorageAccountKey              = self.config_manager.get_config_value("Twin|desired|StorageAccountKey","")
        self.StorageAccountContainer        = self.config_manager.get_config_value("Twin|desired|StorageAccountContainer","deviceuploads")

    def init_schedulers(self):
        """
        Starts schedulers, or reinitializes if already running
        """

        # If the schedulers haven't been enabled yet, stop them if they are already running 
        # (except the watchdog and log uploader)
        # and return
        if (self.schedulers_enabled == False):
            self.stop_schedulers()
            return
        
        # Sensor Capture 
        if (self.sensor_scheduler is not None):
            if ((self.SensorCaptureStartInterval != self.sensor_scheduler.start_interval) or
                  (self.SensorCaptureFrequency != self.sensor_scheduler.interval)):
                self.logger.info("Stopping sensor_scheduler")
                self.sensor_scheduler.stop()
                self.sensor_scheduler = None


        # Image Capture
        if (self.image_scheduler is not None):
            if ((self.ImageCaptureStartInterval != self.image_scheduler.start_interval) or 
                  (self.ImageCaptureFrequency != self.image_scheduler.interval)):
                self.logger.info("Stopping image_scheduler")
                self.image_scheduler.stop()
                self.image_scheduler = None

        # Video Capture
        if (self.video_scheduler is not None):
            if ((self.VideoCaptureStartInterval != self.video_scheduler.start_interval) or 
                  (self.VideoCaptureFrequency != self.video_scheduler.interval)):
                self.logger.info("Stopping video_scheduler")
                self.video_scheduler.stop()
                self.video_scheduler = None

        # Video Uploads
        if (self.video_upload_scheduler is not None):
            if ((self.VideoUploadStartInterval != self.video_upload_scheduler.start_interval) or 
                  (self.VideoUploadSchedulerFrequency != self.video_upload_scheduler.interval)):
                self.logger.info("Stopping video_upload_scheduler")
                self.video_upload_scheduler.stop()
                self.video_upload_scheduler = None

        # Log Uploads
        if (self.log_upload_scheduler is not None):
            if ((self.LogUploadStartInterval != self.log_upload_scheduler.start_interval) or 
                  (self.LogUploadSchedulerFrequency != self.log_upload_scheduler.interval)):
                self.logger.info("Stopping log_upload_scheduler")
                self.log_upload_scheduler.stop()
                self.log_upload_scheduler = None

        # Watchdog
        if (self.watchdog_scheduler is not None):
            if ((self.WatchdogStartInterval != self.watchdog_scheduler.start_interval) or 
                  (self.WatchdogFrequency != self.watchdog_scheduler.interval)):
                self.logger.info("Stopping watchdog_scheduler")
                self.watchdog_scheduler.stop()
                self.watchdog_scheduler = None

        # If the schedulers don't exist yet (or were set the to None above)
        # Create them with the right frequency IF there desired frequence is greater than zero (0)
        if (self.sensor_scheduler is None and self.SensorCaptureFrequency > 0):
            self.logger.info("Starting sensor_scheduler (start_interval, interval): (%d,%d)" % (self.SensorCaptureStartInterval,self.SensorCaptureFrequency))
            self.sensor_scheduler = scheduler(self.SensorCaptureStartInterval, self.SensorCaptureFrequency, self.sensor_callback)
        if (self.image_scheduler is None and self.ImageCaptureFrequency > 0):
            self.logger.info("Starting image_scheduler (start_interval, interval): (%d,%d)" % (self.ImageCaptureStartInterval,self.ImageCaptureFrequency))
            self.image_scheduler = scheduler(self.ImageCaptureStartInterval, self.ImageCaptureFrequency, self.image_callback)
        if (self.video_scheduler is None and self.VideoCaptureFrequency > 0):
            self.logger.info("Starting video_scheduler (start_interval, interval): (%d,%d)" % (self.VideoCaptureStartInterval,self.VideoCaptureFrequency))
            self.video_scheduler = scheduler(self.VideoCaptureStartInterval, self.VideoCaptureFrequency, self.video_callback)
        if (self.video_upload_scheduler is None and self.VideoUploadSchedulerFrequency > 0):
            self.logger.info("Starting video_upload_scheduler (start_interval, interval): (%d,%d)" % (self.VideoUploadStartInterval,self.VideoUploadSchedulerFrequency))
            self.video_upload_scheduler = scheduler(self.VideoUploadStartInterval, self.VideoUploadSchedulerFrequency, self.upload_video)
        if (self.log_upload_scheduler is None and self.LogUploadSchedulerFrequency > 0):
            self.logger.info("Starting log_upload_scheduler (start_interval, interval): (%d,%d)" % (self.LogUploadStartInterval,self.LogUploadSchedulerFrequency))
            self.log_upload_scheduler = scheduler(self.LogUploadStartInterval, self.LogUploadSchedulerFrequency, self.upload_log)
        if (self.watchdog_scheduler is None and self.WatchdogFrequency > 0):
            self.logger.info("Starting watchdog_scheduler (start_interval, interval): (%d,%d)" % (self.WatchdogStartInterval,self.WatchdogFrequency))
            self.watchdog_scheduler = scheduler(self.WatchdogStartInterval, self.WatchdogFrequency, self.watchdog_callback)
            
    def stop_schedulers(self, stop_all=False):
        """
        Stops the schedulers.  Does not stop the watchdog, video upload, or log upload schedulers unless stop_all = True
        """
        if(self.sensor_scheduler is not None):
            self.logger.info("Stopping sensor_scheduler")
            self.sensor_scheduler.stop()
            self.sensor_scheduler = None
        if(self.image_scheduler is not None):
            self.logger.info("Stopping image_scheduler")
            self.image_scheduler.stop()
            self.image_scheduler = None
        if(self.video_scheduler is not None):
            self.logger.info("Stopping video_scheduler")
            self.video_scheduler.stop()
            self.video_scheduler = None
        #Only stop the video uploader, log uplaoder, and watchdog if requested
        if (stop_all==True):
            if(self.video_upload_scheduler is not None):
                self.logger.info("Stopping video_upload_scheduler")
                self.video_upload_scheduler.stop()
                self.video_upload_scheduler = None
            if(self.log_upload_scheduler is not None):
                self.logger.info("Stopping log_upload_scheduler")
                self.log_upload_scheduler.stop()
                self.log_upload_scheduler = None
            if(self.watchdog_scheduler is not None):
                self.logger.info("Stopping watchdog_scheduler")
                self.watchdog_scheduler.stop()
                self.watchdog_scheduler = None
          
    def watchdog_update(self):     
        """
        Updates the watchdog's last_watchdog_time value
        """
        self.last_watchdog_time = time.time()
        pass

    def watchdog_callback(self):
        """
        Call back for the watchdog_scheduler.  Verifies that the current time minuse the last_watchdog_time does not exceed the WatchdogMinRebootTime parameter from the device twin.
        If it does, the device is rebooted
        """
        #Only check for reboot if we have valid values for the last_watchdog_time and the WatchdogMinRebootTime
        if ((self.WatchdogMinRebootTime > 0) and (self.last_watchdog_time > 0)):
            time_difference = time.time() - self.last_watchdog_time
            if (time_difference > self.WatchdogMinRebootTime):
                self.logger.critical('Watchdog exceeded WatchdogMinRebootTime (current time / last_watchdog_time / difference / WatchdogMinRebootTime): ' + str(time.time()) + ' / ' + str(self.last_watchdog_time) + ' / ' + str(time_difference)+ ' / ' + str(self.WatchdogMinRebootTime))
                self.reboot('Watchdog exceeded min reboot time')

    def reboot(self, reason):
        """
        Used to reboot the device.  This is called by the firmware update, model update, and watchdog
        """
        self.logger.critical('Reboot Method Invoked: ' + reason)
        #save the reboot reason and time to the local config file so we can 
        #retrieve and report it on the next boot
        rebootTime = (str(datetime.now().isoformat()) + 'Z')
        # The calls to config_manager.set_config_value have to be wrapped in a try catch in case the saving to disk fails
        try:
            self.config_manager.set_config_value("rebootReason",reason)
        except Exception as ex:
            self.logger.error("There was an error saveing the reboot reason to the config file: {0}".format(str(ex)))
        try:
            self.config_manager.set_config_value("rebootTime",rebootTime)
        except Exception as ex:
            self.logger.error("There was an error saveing the reboot time to the config file: {0}".format(str(ex)))
        # Still try to report it to the cloud now if we can
        # This may fail if being rebooted by the watchdog because of no internet connectivity
        self.set_reported_property('rebootReason', reason)
        self.set_reported_property('rebootTime', rebootTime )
        self.set_reported_property('deviceStatus','rebooting')
        #stop the schedulers, and reboot
        self.stop_schedulers()
        os.system('(sleep 5; sudo shutdown -r now) &')

    def upload_video(self):
        """
        Callback for the video upload scheduler.  Locates the oldest (if any) videos/*.mp4 and videos/*.mp4.json pair
        uploads the video to blob storage, and sends a message to iot hub based on the .json file contents
        """
        if(self.is_video_uploading):
            return
        elif(self.blob_service is None):
            self.logger.error("Videos can't be uploaded because the Azure Blob Service client is not properly configured")
            return
        try:
            # Set the flag to keep another upload from running concurrently
            # This flag will be set back to false when the file is done uploading, or errors out by the blob_upload_callback
            self.is_video_uploading = True

            # Get the list of files, oldest file first
            file_list = sorted(glob.glob(self.SCRIPT_DIR + '/videos/*.mp4'), key=str.lower, reverse=False)

            # If there are no files to upload, return.
            if not len(file_list):
                self.is_video_uploading = False
                return

            # Get the single oldest file from the list
            file_path = file_list[0]

            # The the size of the file
            file_size = os.path.getsize(file_path)

            # Remove the json file for the corresponding video
            json_path = "{0}.json".format(file_path)

            # Determine the destination blob name
            folder = "videos"
            file_name = os.path.basename(file_path)
            blob_name = "{0}/{1}/{2}".format(self.DeviceId, folder ,file_name)

            # Create the context string for the hub_manager.upload_to_blob context
            ctx = {
                "startTime":   time.time(),
                "fileType":    "video",
                "filePath":    file_path,
                "fileSize":    file_size,
                "blobName":    blob_name,
                "contentType": "video/mp4",
                "jsonPath":    json_path
            }

            # Convert to context object to a string so we can pass it
            user_context = json.dumps(ctx)

            self.upload_to_blob_storage(user_context)
            
        except Exception as ex:
            self.is_video_uploading = False
            self.logger.error("UPLOAD: An error occurred when uploading a video file: %s\n" % str(ex))
    
    def upload_log(self):
        """
        Callback for the log upload scheduler.  Locates the oldest (if any) logs/*.log file
        and uploads it to blob storage
        """
        if(self.is_log_uploading):
            return
        elif(self.blob_service is None):
            self.logger.error("Logs can't be uploaded because the Azure Blob Service client is not properly configured")
            return
        try:
            # Set the flag to keep another upload from running concurrently
            # This flag will be set back to false when the file is done uploading, or errors out by the blob_upload_callback
            self.is_log_uploading = True

            # Get the list of files, oldest file first
            file_list = sorted(glob.glob(self.SCRIPT_DIR + '/logs/*.log'), key=str.lower, reverse=False)

            # If there are no files to upload, return.
            # Consider adding something like len(file_list) > 2 if we need to keep a couple of them on the device at all times
            if not len(file_list):
                self.is_log_uploading = False
                return

            # Get the single oldest file from the list
            file_path = file_list[0]

            # The the size of the file
            file_size = os.path.getsize(file_path)

            # Determine the destination blob name
            folder = "logs"
            file_name = os.path.basename(file_path)
            blob_name = "{0}/{1}/{2}".format(self.DeviceId, folder ,file_name)

            # Create the context string for the hub_manager.upload_to_blob context
            ctx = {
                "startTime": time.time(),
                "fileType":"log",
                "filePath": file_path,
                "fileSize": file_size,
                "blobName": blob_name,
                "contentType": "text/plain",
                "jsonPath": ""
            }

            # Convert to context object to a string so we can pass it
            user_context = json.dumps(ctx)

            #self.upload_to_blob(blob_name, open(file_path, 'rb').read(), file_size, user_context)
            #self.upload_to_blob_storage(blob_name, open(file_path, 'rb').read(), file_size, user_context)
            self.upload_to_blob_storage(user_context)
        except Exception as ex:
            self.is_log_uploading = False
            self.logger.error("UPLOAD: An error occurred when uploading a log file: %s\n" % str(ex))
 
    def upload_to_blob_storage(self, user_context):
        """
        Uploads a blob to blob storage. The source file, destination blob, and addtional details are passed in the user_context 
        """
        try:
            ctx = {}
            try:
                ctx = json.loads(user_context)
                start_time = ctx["startTime"]
                file_type = ctx["fileType"]
                file_path = ctx["filePath"]
                file_size = ctx["fileSize"]
                blob_name = ctx["blobName"]
                content_type = ctx["contentType"]
                # Not every file will have an associated json message payload...
                # For example, video files have a .mp4 and a .m4p.json file
                # Log files however are just a single .log file, nothing else.
                try:
                    json_path = ctx["jsonPath"]
                except:
                    json_path = ""
            except:
                self.logger.error("UPLOAD: Error parsing user_context: %s" % user_context)
                ctx["user_context"] = user_context
                start_time = time.time()
                file_type = ""
                file_path = ""
                file_size = 0
                blob_name = ""
                content_type= "application/octet-stream"
                json_path = ""

            self.logger.debug("UPLOAD: user_context %s " % user_context)

            if(file_path==""):
                self.logger.error("UPLOAD: Upload aborting because the user_context is invalid: %s" % user_context)
                return

            
            # This runs synchronously, not async...
            content_settings = ContentSettings(content_type=content_type) 
            content_metadata = {}            
            self.logger.info("UPLOAD: Uploading %s file (%d bytes): %s / %s" % (file_type,file_size,file_path,blob_name))
            self.blob_service.create_blob_from_path(self.StorageAccountContainer, blob_name, file_path, content_settings = content_settings, metadata = content_metadata)

            elapsed_time = time.time() - start_time

            self.logger.info("UPLOAD: Successfully uploaded %s %s (%d bytes) in %f seconds (%d bytes/sec)" % (file_type,file_path,file_size,elapsed_time,(file_size/elapsed_time)))

            if(json_path != ""):
                # There is a JSON file with an associated message in it that needs to be sent.
                # Send the use the iothub client to send the message, and pass the path to the file we just uploaded (file_path)
                #  and the path to the JSON file with the message along in the user context to the send_event_async method so they 
                #  can be deleted when the send succeeds.
                try:
                    iothub_message = json.load(open(json_path))
                    send_context = json.dumps({
                        'clearIsVideoUploading': True,
                        'filesToRemove': [file_path,json_path]
                    })
                    payload = None
                    properties = None
                    try:
                        # Get the payload as a string
                        payload = json.dumps(iothub_message["payload"])
                        # Get the properties as a dictionary
                        properties = iothub_message["properties"]
                    except:
                        if (payload is None):
                            self.logger.error("UPLOAD: They payload for the message to send to iot hub is empty")
                    if(payload is not None):
                        self.logger.info("UPLOAD: Sending message from file: %s" % json_path)
                        self.send_event(payload, properties, send_context)
                except Exception as ex:
                    self.logger.error("UPLOAD: There was an error sending the message based on the file: %s\n%s" % (json_path,str(ex)))

            else:
                # There is no corresponding JSON file for a message to send after the file upload above succeeded 
                # Therefore, we can just delete the file we successfully uploaded above.
                self.logger.debug("UPLOAD: Removing uploaded file: %s" % file_path)
                os.remove(file_path)

        except Exception as ex:
            self.logger.error("UPLOAD: An exception occurred when attempting to upload file %s\n%s" % (file_path,str(ex)))
        finally:
            # If this is a video file, don't clear the self.is_video_uploading flag yet.  
            # We won't do that until the associated media message has been sent and the file has been removed. 
            # If this is log file, set the is_log_uploading to false so another log can be uploaded
            if (file_type == "log"):
                self.is_log_uploading = False

    def send_event(self, event, properties, send_context):
        """
        Sends an event to the IoT Hub, passing along the send_context to the IoT Hub client callback.
        """
        if not isinstance(event, IoTHubMessage):
            event = IoTHubMessage(bytearray(event, 'utf8'))
        if(event.message_id is None):
            event.message_id = str(uuid.uuid4())
        if len(properties) > 0:
            prop_map = event.properties()
            for key in properties:
                prop_map.add_or_update(key, properties[key])
        self.client.send_event_async(
            event, self.send_confirmation_callback, send_context)

    def send_confirmation_callback(self, message, result, send_context):
        """
        Called by the IoT Hub client after a Device to Cloud (D2C) message was sent or rejected.  Uses the send_context to clean up any files and flags associated with the message.
        """
        result_str = str(result).strip().upper()
        if result_str == "OK":
            # Update the watchdog
            self.watchdog_update()

            # See if there is a list of files in send_context that we should remove now that the message was sent successfully...
            try:
                ctx = json.loads(send_context)
                files_to_remove = ctx["filesToRemove"]
                try:
                    clear_is_video_uploading = ctx["clearIsVideoUploading"]
                except:
                    clear_is_video_uploading = False
            except:
                files_to_remove = []
            if(len(files_to_remove) > 0):
                for path in files_to_remove:
                    try:
                        self.logger.debug("D2CMESSAGE: Removing file %s" % path)
                        os.remove(path)
                    except Exception as ex:
                        self.logger.error("D2CMESSAGE: Error removing file %s\n%s" % (path,str(ex)))
                    # If we have been told to clear the is_video_uploading flag after removing the files, clear it.
                    if (clear_is_video_uploading == True):
                        self.is_video_uploading = False

            # If we are logging at the debug level, log success info about the message
            if(self.logger.getEffectiveLevel() == 10):
                map_properties = message.properties()
                key_value_pair = map_properties.get_internals()
                self.logger.debug("D2CMESSAGE: Successfully sent message (message_id,properties): (%s,%s)" % (message.message_id,key_value_pair))
        elif result_str == "ERROR":
            self.logger.error("D2CMESSAGE: An error occurred sending a device-to-cloud message")

    def receive_message_callback(self, message, counter):
        """
        Called by the IoT Hub client when a Cloud to Device (C2D) message is received.
        """
        message_buffer = message.get_bytearray()
        size = len(message_buffer)
        self.logger.debug("Received Message [%d]:" % counter)
        self.logger.debug("Data: <<<%s>>> & Size=%d" %
                    (message_buffer[:size].decode('utf-8'), size))
        map_properties = message.properties()
        key_value_pair = map_properties.get_internals()
        self.logger.debug("Properties: %s" % key_value_pair)
        counter += 1
        return IoTHubMessageDispositionResult.ACCEPTED

    def device_twin_callback(self, update_state, payload, user_context):
        """
        Called by the IoT Hub client when a device twin update (partial or complete) is received. The variables, logger and schedulers are updated based on received values
        """
        self.logger.info("TWINUPDATE: %s twin update received:\npayload = %s" % (update_state, payload))
        try:
            self.config_manager.twin_update(update_state, payload)
        except Exception as ex:
            self.logger.error("There was an error saving the twin update to the config file:\n{0}".format(str(ex)))
        self.set_variables_from_config()
        # Reset the logging handlers
        self.init_logger()
        # Reset the Azure Blob Service Client
        self.init_azure_storage()
        # Re-initialize the schedulers
        self.init_schedulers()