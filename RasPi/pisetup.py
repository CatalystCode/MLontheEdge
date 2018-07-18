#!/usr/bin/env python3

import os
import zipfile
import logging
from azure.storage.blob import BlockBlobService, ContentSettings, PublicAccess

SCRIPT_DIR = os.path.split(os.path.realpath(__file__))[0]

def azure_download_from_path(model_container_name, model_dir_path, compressed_model_dir_path, compressed_model_name):
    #Download Azure Version to the Raspberry pi
        block_blob_service.get_blob_to_path(model_container_name, compressed_model_name, compressed_model_dir_path)
        os.makedirs(model_dir_path)
        zf = zipfile.ZipFile(compressed_model_dir_path)
        zf.extractall(model_dir_path)

def main():
    # Define Globals
    global block_blob_service

    # Define Locals
    model_container_name = 'edgemodels'
    current_dir = os.getcwd()
    model_dir = "pi3"
    build_dir = "build"
    model_dir_path = "{0}/{1}".format(SCRIPT_DIR, model_dir)
    build_dir_path = "{0}/{1}".format(model_dir_path, build_dir)
    compressed_model_name = "zipped{0}".format(model_dir)
    compressed_model_dir_path ="{0}/{1}.zip".format(SCRIPT_DIR, compressed_model_name)
   
    # Set up Azure Credentials
    block_blob_service = BlockBlobService(account_name='***************', account_key='***************************')
    if block_blob_service is None:
        logging.debug("No Azure Storage Account Connected")
        sys.exit(1)
    
    # Download Pi3 from Azure
    if not os.path.exists(model_dir_path):
        azure_download_from_path(model_container_name, model_dir_path, compressed_model_dir_path, compressed_model_name)
        # Check one more time to make sure that now the correct directory has been downloaded
        if os.path.exists(model_dir_path):
            os.chdir(model_dir_path)

    # Create a build Folder
    while not os.path.exists(build_dir_path):
        os.makedirs(build_dir_path)
   
    # Change into the 'build' folder
    os.chdir(build_dir_path)
    
    # Call the OS to run the 'cmake' command
    os.system('cmake .. -DCMAKE_BUILD_TYPE=Release')

    # Call the OS to run the essential 'make' command
    os.system('make')

    # Change back into our current Scripts Directory
    os.chdir(current_dir)

if __name__ == '__main__':
    main()
