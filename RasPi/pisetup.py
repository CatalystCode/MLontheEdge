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
    print('Printing ScriptDir:')
    print(SCRIPT_DIR)

    # Define Globals
    global block_blob_service

    # Define Locals
    current_dir = os.getcwd()
    print('Printing the current dir before changing')
    print(current_dir)

    model_container_name = 'edgemodels'
    model_dir = "pi3"
    build_dir = "build"
    model_dir_path = "{0}/{1}".format(SCRIPT_DIR, model_dir)
    build_dir_path = "{0}/{1}".format(model_dir_path, build_dir)
    compressed_model_name = "zipped{0}".format(model_dir)
    compressed_model_dir_path ="{0}/{1}.zip".format(SCRIPT_DIR, compressed_model_name)
   
    # Set up Azure Credentials
    block_blob_service = BlockBlobService(account_name='*************', account_key='*******************************')
    if block_blob_service is None:
        logging.debug("No Azure Storage Account Connected")
        sys.exit(1)
    
    # Download Pi3 from Azure
    print('About to download the pi3 folder from Azure')
    azure_download_from_path(model_container_name, model_dir_path, compressed_model_dir_path, compressed_model_name)
    print('I have downloaded the pi3 folder form Azure')

    # Change into the Pi3 folder
    print('About to change into the pi3 folder')
    os.chdir(model_dir_path)
    print('I have changed into the pi3 folder')

    # Create a build Folder
    print('Creating the Build Folder')
    while not os.path.exists(build_dir_path):
        os.makedirs(build_dir_path)
    print('Created the Build Folder')
   
    # Change into the 'build' folder
    print('About to change into the Build Folder')
    os.chdir(build_dir_path)
    print('I have changed into the build folder')
    
    # Call the OS to run the 'cmake' command
    print('About to make the cmake call in the build folder')
    os.system('cmake')
    print ('I have successfully ran the cmake command')

    # Call the OS to run the essential 'make' command
    print('About to run the make command on the OS')
    os.system('make')
    print('We successfully ran make on the OS. WOW')

    # Change back into our current Scripts Directory
    print('Changing back into the current directory')
    os.chdir(current_dir)
    print('We have changed back into our working directory')
    print('We are going back to our original code')

if __name__ == '__main__':
    main()
