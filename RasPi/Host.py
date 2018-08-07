#!/usr/bin/env python3

import json
import os
import shutil
import sys
import time
import zipfile
from datetime import datetime, timedelta
from azure.storage.blob import BlockBlobService, ContentSettings, PublicAccess

SCRIPT_DIR = os.path.split(os.path.realpath(__file__))[0]

def azure_upload_from_path(blob_container,blob_name,blob_object,blob_format):
        block_blob_service.create_blob_from_path(blob_container, blob_name,blob_object, content_settings=ContentSettings(content_type=blob_format))


def main():
    # Define Globals
    global block_blob_service
    
    # Define Locals
    model_dir = "pi3"
    model_dir_path = "{0}/{1}".format(SCRIPT_DIR, model_dir)
    compressed_model_name = "zipped{0}".format(model_dir)
    model_container_name = 'edgemodels'
    compressed_model_dir_path ="{0}/{1}.zip".format(SCRIPT_DIR, compressed_model_name)
    
    # Initialize Azure Connection
    block_blob_service = BlockBlobService(account_name='*************', account_key='***************************************')
    
    # Create Blob Container if it doesn't already exists
    block_blob_service.create_container(model_container_name)

    # Compress the current model file on the computer
    if os.path.exists(model_dir_path):
        shutil.make_archive(compressed_model_name, 'zip', model_dir_path)
    else:
        print("There is no model file in this directory")
        sys.exit(1)

    # Upload the Compressed file to Azure
    azure_upload_from_path(model_container_name, compressed_model_name, compressed_model_dir_path, 'application/zip')

if __name__ == '__main__':
    main()
