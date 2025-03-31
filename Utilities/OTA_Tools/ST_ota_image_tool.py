#!/usr/bin/env python3

"""
Matter OTA (Over-the-air update) image utility.

This script relies on the script ota_image_tool.py script (Copyright (c) 2023 Project CHIP Authors).
It can be used to automatically parse the CHIP configuration file.

Usage examples:

Creating OTA image file:
./ST_ota_image_tool.py -cc CHIPProjectConfig.h -vs "my-new-version-string" my-firmware.bin my-firmware.ota 

"""

import re
import argparse
import sys

# === import chiptool libraries ===
from ota_image_tool import validate_header_attributes as OTA_validate_header_attributes
from ota_image_tool import generate_image as OTA_generate_image

# #################################################################
#  class HeaderAttributes
#  This class is the image of args structure used by ota_image_tool
# #################################################################
class HeaderAttributes:
    def __init__(self,
                 input_files, output_file,
                 vendor_id, product_id, version, version_str, digest_algorithm, 
                 min_version=None, max_version=None, release_notes=None, ):
        
        self.vendor_id = vendor_id
        self.product_id = product_id
        self.version = version		
        self.version_str = version_str
        self.digest_algorithm = digest_algorithm
        self.min_version = min_version
        self.max_version = max_version
        self.release_notes = release_notes
        self.input_files = input_files
        self.output_file = output_file

    def print(self):
        print("HeaderAttributes parameters:")
        print(f"vendor_id = {self.vendor_id}")
        print(f"product_id = {self.product_id}")
        print(f"version = {self.version}")	
        print(f"version_str = {self.version_str}")
        print(f"digest_algorithm = {self.digest_algorithm}")
        print(f"min_version = {self.min_version}")
        print(f"max_version = {self.max_version}")
        print(f"release_notes = {self.release_notes}")
        print(f"input_files = {self.input_files}")
        print(f"output_file = {self.output_file}")

#end of class HeaderAttributes

# #################################################################
#  extract_data_from_chip_config function
#  This function extract data from the CHIP configuration file
#  (CHIPProjectConfig.h)       
#  Returns: VENDOR_ID, PRODUCT_ID, SW_VERSION
# #################################################################
def extract_data_from_chip_config(file_chip_config):

    val_vendor_id = "INVALID_VALUE"
    val_product_id = "INVALID_VALUE"
    val_sw_version = "INVALID_VALUE"

    try:
        with open(file_chip_config, 'r') as f:
            chip_config_file = f.read()            
            f.close()

            # split the content in a list of lines
            lines = chip_config_file.splitlines()

            # Extract infos
            for line in lines:
                if line.startswith('#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID'):
                    match = re.search(r'(\S+)$', line)
                    if match:
                        val_vendor_id = match.group(1)                

                elif line.startswith('#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID'):
                    match = re.search(r'(\S+)$', line)
                    if match:
                        val_product_id = match.group(1)

                elif line.startswith('#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION '):
                    # Note: keep the space after the word VERSION to distinguish this case from VERSION_STRING
                    match = re.search(r'(\S+)$', line)
                    if match:
                        val_sw_version = match.group(1)

    except:
        print("Error opening CHIPProjectConfig.h")

    return val_vendor_id, val_product_id, val_sw_version
#end of function extract_data_from_chip_config


# #################################################################
#  parse_input_args function
#  This function parse input arguments
#  Returns: HeaderAttributes object
# #################################################################
def parse_input_args():

    def any_base_int(s): return int(s, 0)

    # arguments parsing
    parser = argparse.ArgumentParser(description='Parmaters parsing')

    # parameter: path to CHIPProjectConfig.h to extract automatically Vendor ID, Product ID, SW version and
    #            SW version string.
    parser.add_argument('-cc', '--chip-config', type=str,
                               help='path to CHIPProjectConfig.h')
    
    # parameters: used to specify explicitly some parameters 
    # Will override value extracted from CHIPProjectConfig.h if specified
    parser.add_argument('-v', '--vendor-id', type=any_base_int,
                               help='Vendor ID')
    parser.add_argument('-p', '--product-id', type=any_base_int,
                               help='Product ID')
    parser.add_argument('-vn', '--version', type=any_base_int,
                               help='Software version (numeric)')
    parser.add_argument('-vs', '--version-str', type=str, required=True,
                               help='Software version (string)')    
    parser.add_argument('-da', '--digest-algorithm',
                               help='Digest algorithm')    

    # parameters
    parser.add_argument('input_file', help='Path to input image payload file')
    parser.add_argument('output_file', help='Path to output image file')

    # default values
    vendor_id = product_id = sw_version = sw_version_string = None
    digest_algorithm = "sha256"
    min_version = None
    max_version = None
    release_notes = None

    # parse arguments    
    args = parser.parse_args()

    print(args)

    # extract data from CHIP config file if specified
    if args.chip_config is not None:
        vendor_id, product_id, sw_version = extract_data_from_chip_config(args.chip_config)

    if args.vendor_id is not None:
        vendor_id = args.vendor_id

    if args.product_id is not None:
        product_id = args.product_id

    if args.version is not None:
        sw_version = args.version

    if args.version_str is not None:
        sw_version_string = args.version_str              

    if args.digest_algorithm is not None:
        digest_algorithm = args.digest_algorithm    

    # checkings    
    if (vendor_id is None) or (product_id is None) or (sw_version is None):
        sys.stderr.write('error: invalid arguments\n')
        sys.exit(1)

    build_ota_args = HeaderAttributes(
        [args.input_file], 
        args.output_file,
        int(vendor_id, 16),
        int(product_id, 16),
        int(sw_version),
        sw_version_string,
        digest_algorithm,
        min_version,
        max_version,
        release_notes)

    build_ota_args.print()

    return build_ota_args
# end of function parse_input_args  

# #################################################################
#  main function
# #################################################################
if __name__ == "__main__":
    """ Main starts here """

    ota_args = parse_input_args()

    print("-=-=-=-=-=-=-=-=-=-=-")

    # call CHIPTOOL functions
    OTA_validate_header_attributes(ota_args)    
    print("OTA: validate_header_attributes done.")

    OTA_generate_image(ota_args)                  
    print("OTA: generate_image done.")   

    print("-=-=-=-=-=-=-=-=-=-=-")
