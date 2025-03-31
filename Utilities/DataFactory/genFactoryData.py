# Copyright(c) 2024 STMicroelectronics International N.V.
# Copyright 2017 Linaro Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import inspect
import struct
import json
import argparse
import subprocess
import tempfile
import os
import re
import ssl
from enum import Enum, auto
import serial
import serial.tools.list_ports
import concurrent.futures
import threading
from datetime import datetime

import common
from common import RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, RESET
import importlib.util

# STM32CubeProgrammer path and executable file name
programmer = {
    'path' : "c:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin",
    'prg' : "STM32_Programmer_CLI.exe"
}
# STM32CubeProgrammer external loader driver for external flash path and driver name
external_loader = {
    'path' : programmer['path'] + "\\" + "ExternalLoader",
    'drv' : "S25FL128S_STM32WB5MM-DK"
}

class dataType(Enum):
    INT8 = 1
    INT16 = auto()
    INT32 = auto()
    STRING = auto()
    ARRAY8 = auto()

class flashingType(Enum):
    DATAFACTORY = 1
    APPLICATION  = auto()
    COPROCESSOR = auto()

class cryptoType(Enum):
    CERT = 1
    PUBLIC = auto()
    PRIVATE = auto()

# Dictionary to map names to types and IDs
name_map = {
    # DeviceAttestationCredentialsProvider
    "CERTIFICATION_DECLARATION": {"type": dataType.ARRAY8, "id": 1},
    "FIRMWARE_INFORMATION": {"type": dataType.ARRAY8, "id": 2},
    "DEVICE_ATTESTATION_CERTIFICATE": {"type": dataType.ARRAY8, "id": 3},
    "PAI_CERTIFICATE": {"type": dataType.ARRAY8, "id": 4},
    "DEVICE_ATTESTATION_PRIV_KEY": {"type": dataType.ARRAY8, "id": 5},
    "DEVICE_ATTESTATION_PUB_KEY": {"type": dataType.ARRAY8, "id": 6},
    # CommissionableDataProvider
    "SETUP_DISCRIMINATOR": {"type": dataType.INT16, "id": 11},
    "SPAKE2_ITERATION_COUNT": {"type": dataType.INT32, "id": 12},
    "SPAKE2_SALT": {"type": dataType.ARRAY8, "id": 13},
    "SPAKE2_VERIFIER": {"type": dataType.ARRAY8, "id": 14},
    "SPAKE2_SETUP_PASSCODE": {"type": dataType.INT32, "id": 15},
    # DeviceInstanceInfoProvider
    "VENDOR_NAME": {"type": dataType.STRING, "id": 21},
    "VENDOR_ID": {"type": dataType.INT16, "id": 22},
    "PRODUCT_NAME": {"type": dataType.STRING, "id": 23},
    "PRODUCT_ID": {"type": dataType.INT16, "id": 24},
    "SERIAL_NUMBER": {"type": dataType.STRING, "id": 25},
    "MANUFACTURING_DATE": {"type": dataType.STRING, "id": 26},
    "HARDWARE_VERSION": {"type": dataType.INT16, "id": 27},
    "HARDWARE_VERSION_STRING": {"type": dataType.STRING, "id": 28},
    "ROTATING_DEVICE_ID": {"type": dataType.STRING, "id": 29},
    # Platform specific
    "TAG_ID_ENABLE_KEY": {"type": dataType.STRING, "id": 41}
}

paramToName = {key.lower().replace("_", ""): key for key, value in name_map.items()}

baud_rate = 115200
MAGIC_KEYWORD = 0xACACACAC

# Define global locks
file_json_lock = threading.Lock()
file_binary_lock = threading.Lock()

# Replaces the extension of the given file path with a new extension and appends
# a discrinant identifier.
def replace_extension_with(file_path, disc, ext):
    # Split the file path into root and extension
    root, _ = os.path.splitext(file_path)
    # Create the new file path with the new extension
    new_file_path = f"{root}_{disc}.{ext}"
    # Check if the new file path already exists and modify it if necessary
    counter = 1
    while os.path.exists(new_file_path):
        new_file_path = f"{root}_{disc}_{counter}.{ext}"
        counter += 1
    return new_file_path

# Function to convert value from decimal or hexadecimal format
def convert_value(value):
    # Check if the value starts with "0x"
    if value.startswith("0x"):
        # If it does, assume it's a hexadecimal number and convert it using base 16
        return int(value, 16)
    else:
        # If it doesn't start with "0x", assume it's a decimal number and convert it using base 10
        return int(value)

# Converts a bytes object to a hexadecimal string and returns it along with the integer representation.
def bytes_to_hex_string(my_bytes, format_specifier):
    # Convert the bytes object to an integer using little-endian byte order
    my_int = int.from_bytes(my_bytes, byteorder='little')

    # Convert the integer to a hexadecimal string with the specified format specifier
    my_hex_string = format(my_int, format_specifier)

    # Return the hexadecimal string prefixed with '0x' and the integer representation
    return ("0x" + my_hex_string, my_int)

def sort_key(port_info):
    # This regular expression will match the letters "COM" followed by a number
    match = re.match(r'(COM)(\d+)', port_info['port'])
    if match:
        # Return a tuple with the text part and the numerical part as an integer
        return (match.group(1), int(match.group(2)))
    return (port_info['port'],)

# Lists available serial ports by running a command specific to the given programmer.
def list_serial_ports(programmer):
    # Store the command in a variable
    command = [
            programmer['path'] + "\\" + programmer['prg'], "-l"]
    # Run the command and capture the output
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    # Extract the "=====  UART Interface  =====" section  from the command output
    uart_section_match = re.search(r'=====  UART Interface  =====(.*?)$', result.stdout, re.DOTALL)

    if uart_section_match:
        # If the section is found, extract the content
        uart_section = uart_section_match.group(1)

        # Define a regex pattern to extract tuples containing Board Name, ST-LINK SN, and Port
        pattern = re.compile(r'Board Name\s*:\s*([^\s\r\n]+)\nST-LINK SN:\s*([^\s\r\n]+)\nPort:\s*([^\s\r\n]+)\n', re.DOTALL)
        matches = pattern.findall(uart_section)

        # Convert the port strings to integers and sort the list of tuples by the port
        sorted_matches = sorted(matches, key=lambda x: int(re.search(r'\d+', x[2]).group()))

        # Convert the sorted tuples into a list of dictionaries
        available_ports = [
            {
                'uart_device': match[0],
                'uart_serial': match[1],
                'uart_port': match[2]
            }
            for match in sorted_matches
        ]
    else:
        # If the section is not found, return an empty list
        available_ports = []
    return available_ports

# Checks if a given serial port is available by attempting to open it.
def check_port_availability(serial_port, baud):
    try:
        # Attempt to open the serial port with the specified baud rate and a timeout of 1 second
        ser = serial.Serial(serial_port, baud, timeout=1)
        # If no exception is raised, the port is available
        # Close the port immediately after opening it
        ser.close()
        return True
    except serial.SerialException:
        # If an exception is raised, the port is likely in use or cannot be accessed
        return False

# Displays a message indicating whether a value has changed or been set for a given identifier.
def displayChangedValue(header, data_dict, who, name, id, value, format=None):
    # Define a separator string to use in output messages
    sep = '  '

    # Check if the format parameter is 'string'
    if format == 'string':
        # Check if the id parameter is in the data_dict dictionary
        if id in data_dict:
            # Check if the value associated with the id key has changed
            if data_dict[id][0] != value:
                # Print a message indicating that the value has changed
                common.thread_safe_print(f"{sep}{header}{who} changed {CYAN}{name}{RESET} from {data_dict[id][0]} to {value}")
            else:
                # Print a message indicating that the value has not changed
                common.thread_safe_print(f"{sep}{header}{who} set value for existing {CYAN}{name}{RESET} with no change : {value}")
        else:
            # Print a message indicating that a new value has been set
            common.thread_safe_print(f"{sep}{header}{who} set value for {CYAN}{name}{RESET}: {value}")

    # Check if the format parameter is 'none'
    elif format == 'none':
        if id in data_dict:
            # Check if the value associated with the id key has changed
            if data_dict[id][0] != value:
                # Print a message indicating that the value has changed
                common.thread_safe_print(f"{sep}{header}{who} changed {CYAN}{name}{RESET} from\n{data_dict[id][0]}\nto\n{value}")
            else:
                # Print a message indicating that the value has not changed
                common.thread_safe_print(f"{sep}{header}{who} set value for existing {CYAN}{name}{RESET} with no change :\n{value}")
        else:
            # Print a message indicating that a new value has been set
            common.thread_safe_print(f"{sep}{header}{who} set value for {CYAN}{name}{RESET}:\n{value}")

    # If the format parameter is not 'string' or 'none'
    else:
        # Check if the id parameter is in the data_dict dictionary
        if id in data_dict:
            # Convert the old and new values to a hex string plus decimal string using the bytes_to_hex_string() function
            old, old_dec = bytes_to_hex_string(data_dict[id][0], format)
            new, new_dec = bytes_to_hex_string(value, format)
            # Check if the old and new values are different
            if old != new:
                # Print a message indicating how the value has changed
                common.thread_safe_print(f"{sep}{header}{who} changed {CYAN}{name}{RESET} from {old} ({old_dec}) to {new} ({new_dec})")
            else:
                common.thread_safe_print(f"{sep}{header}{who} set value for existing {CYAN}{name}{RESET} with no change : {new} ({new_dec})")
        else:
            # Convert the new value to a hex string using the bytes_to_hex_string() function
            new, new_dec = bytes_to_hex_string(value, format)
            # Print a message indicating that a new value has been set
            common.thread_safe_print(f"{sep}{header}{who} set value for {CYAN}{name}{RESET} : {new} ({new_dec})")

def fillData(header, name, value, data_dict, who, verbose):
    try:
        id = name_map[name]["id"]
        type = name_map[name]["type"]
    except KeyError:
        common.thread_safe_print(f"{RED}  {header}{who} can't set value for {name}: {value}.\n{name} is an unknown parameter{RESET}")
        return
    #print(f"  Name: {name}, ID: {id}, Type: {type}")
    # Convert value to appropriate type
    if type == dataType.INT8:
        value = convert_value(value)
        value = struct.pack("B", value)
        if verbose: displayChangedValue(header, data_dict, who, name, id, value, '02x')
    elif type == dataType.INT16:
        value = convert_value(value)
        value = struct.pack("<H", value)
        if verbose: displayChangedValue(header, data_dict, who, name, id, value, '04x')
    elif type == dataType.INT32:
        value = convert_value(value)
        value = struct.pack("<I", value)
        if verbose: displayChangedValue(header, data_dict, who, name, id, value, '08x')
    elif type == dataType.ARRAY8:
        value = bytes(int(x, 16) for x in value)
        if verbose: displayChangedValue(header, data_dict, who, name, id, value, 'none')
    elif type == "float":
        value = float(value)
        value = struct.pack("<f", value)
    elif type == "bool":
        value = bool(value)
        value = struct.pack("<?", value)
    else:
        value = str(value).encode("utf-8")
        if verbose: displayChangedValue(header, data_dict, who, name, id, value, 'string')
    # Add ID/value/type tuple to dictionary
    data_dict[id] = (value, type, name)
    #print(f"  Name: {name}, ID: {id}, Type: {type}, Value: {value}")

# Function to read JSON file and fill data_dict
# Open JSON file for reading
# Each item in the data array represents a TLV
# The ID field is a unique identifier for the TLV
# The type field specifies the data type of the value
# Possible types are: int8, int16, int32, string
# The value field contains the value of the TLV
# The name field is an optional field that provides a human-readable name for the TLV

def read_json_file(header, file_path, data_dict, verbose):

    # Open JSON file for reading
    try:
        with open(file_path, "r") as f:
            # Load JSON data
            data = json.load(f)
    except FileNotFoundError:
        common.thread_safe_print(f"{RED}{header}Error: JSON file {file_path} not found{RESET}")
        return None
    except json.JSONDecodeError:
        common.thread_safe_print(f"{RED}{header}Error: JSON file {file_path} is not a valid JSON file{RESET}")
        return None

    # Loop through JSON data
    for name, value in data.items():
        fillData(header, name, value, data_dict, "Read Json", verbose)
    return data_dict

# Function to write data_dict to binary file
def writeBinary(header, data_dict, file_path):
    # Sort data_dict by ID
    data_dict = dict(sorted(data_dict.items()))

    # Open binary file for writing
    try:
        with file_binary_lock:
            with open(file_path, "wb") as f:
                # Loop through ID/value/type tuples
                for id, (value, _, _) in data_dict.items():
                    # Verify that value does not exceed length
                    length = len(value)
                    # Convert ID to 32-bit binary format in native byte order
                    id_bin = struct.pack("I", id)
                    # Convert length to 32-bit binary format in little-endian byte order
                    len_bin = struct.pack("<I", length)
                    # Write TLV to binary file
                    f.write(id_bin + len_bin + value)
                # Convert magic keyworw to 32-bit binary format in native byte order
                id_bin = struct.pack("I", MAGIC_KEYWORD)
                f.write(id_bin)
                common.thread_safe_print(f"{GREEN}{header}Write to binary file{file_path} successful{RESET}")
                return 0
    except IOError as e:
        common.thread_safe_print(f"{RED}{header}Write to binary file {file_path} return ERROR -->{RESET}")
        common.thread_safe_print(e)
        return 1

# This function reads a binary file containing TLVs and returns a dictionary data_dict containing
# the TLVs and their values. The TLVs are identified by their unique IDs and contain values of
# different data types such as integers, floats, strings, and booleans. The function uses the struct
# module to unpack the binary data based on the data type specified in the name_map dictionary.
# The resulting dictionary can be used to access the TLVs and their values.
def readBinary(header, file_path, data_dict, verbose):
    # Open binary file for reading
    try:
        f = open(file_path, "rb")
    except FileNotFoundError:
        common.thread_safe_print(f"{RED}{header}Error: binary file {file_path} not found{RESET}")
        return None

    # Loop through binary data
    while True:
        # Read ID (4 bytes)
        id_bin = f.read(4)
        if not id_bin:
            # End of file
            break
        # Convert ID from binary to integer
        id = struct.unpack("I", id_bin)[0]

        if id != MAGIC_KEYWORD:
            # Read length (4 bytes)
            len_bin = f.read(4)
            # Convert length from binary to integer
            length = struct.unpack("<I", len_bin)[0]

            # Read value (length bytes)
            value_bin = f.read(length)

        # Determine data type based on ID
        if id in [v["id"] for v in name_map.values()]:
            # Get name of TLV from name_map dictionary
            name = [k for k, v in name_map.items() if v["id"] == id][0]
            # Get data type from name_map dictionary
            type = name_map[name]["type"]
            # Unpack value based on data type
            if type == dataType.INT8:
                value = struct.unpack("b", value_bin)[0]
                if verbose: displayChangedValue(header, data_dict, "Read binary", name, id, value_bin, '02x')
            elif type == dataType.INT16:
                value = struct.unpack("<h", value_bin)[0]
                if verbose: displayChangedValue(header, data_dict, "Read binary", name, id, value_bin, '04x')
            elif type == dataType.INT32:
                value = struct.unpack("<i", value_bin)[0]
                if verbose: displayChangedValue(header, data_dict, "Read binary", name, id, value_bin, '08x')
            elif type == "float":
                value = struct.unpack("<f", value_bin)[0]
            elif type == "bool":
                value = struct.unpack("<?", value_bin)[0]
            elif type == dataType.STRING:
                value = value_bin.decode("utf-8")
                if verbose: displayChangedValue(header, data_dict, "Read binary", name, id, value_bin, 'string')
            else:
                value = value_bin
                if verbose: displayChangedValue(header, data_dict, "Read binary", name, id, value_bin, 'none')
            # Add ID/value/type tuple to dictionary
            value = value_bin
            data_dict[id] = (value, type, name)
            #print(f"  Name: {name}, ID: {id}, Type: {type}, Value: {value}")
        elif id == MAGIC_KEYWORD:
            common.thread_safe_print(f"{GREEN}{header}End of binary found{RESET}")
        else:
            common.thread_safe_print(f"{RED}  {header}Read binary Type {id} incorrect, this value will be skipped{RESET}")
    f.close()
    return data_dict

def flashBinary(header, binaryFile, flashAddr, programmer, uart_serial, external_loader, type):

    if type == flashingType.APPLICATION:
        displayStr = "application processor firmware"
    elif type == flashingType.COPROCESSOR:
        displayStr = "coprocessor firmware"
    else:
        displayStr = "data factory"

    if uart_serial is not None:
        # Construct the command to flash the binary file to the specified address
        command = [
            programmer['path'] + "\\" + programmer['prg'], "-c", "port=SWD", "sn=" + uart_serial, "mode=UR",
            "-el", external_loader['path'] + "\\" + external_loader['drv'] + ".stldr",
            "-d", binaryFile, flashAddr, "-V", "--quietMode", "--verbosity 1", "-rst"
        ]
        if type == flashingType.COPROCESSOR:
            # start wireless stack
            command = [programmer['path'] + "\\" + programmer['prg'], "-c", "port=SWD", "sn=" + uart_serial, "mode=UR", "-startwirelessstack",
                       "--quietMode", "--verbosity 1"]
            subprocess.run(command, capture_output=True, text=True)

            command = [
                    programmer['path'] + "\\" + programmer['prg'], "-c", "port=SWD", "sn=" + uart_serial, "mode=UR",
                    "-ob nSWboot0=0 nboot1=1 nboot0=1", "-startfus",
                    "-fwupgrade", binaryFile, flashAddr, "-V", "--quietMode", "--verbosity 1"
                ]

        # Run the command using check_output()
        try:
            if type == flashingType.DATAFACTORY:
                subprocess.check_output(command, stderr=subprocess.STDOUT)
            else:
                subprocess.check_output(command, stderr=subprocess.STDOUT)
            # Print a message indicating that the binary file was successfully flashed to the specified address
            common.thread_safe_print(f"{GREEN}{header}Flash {displayStr} file {binaryFile} to address {flashAddr} successful{RESET}")
        except subprocess.CalledProcessError as e:
            # Print an error message if the flashing failed and print the output of the command
            common.thread_safe_print(f"{RED}{header}Flash {displayStr} file {binaryFile} to address {flashAddr} returned ERROR -->{RESET}")
            common.thread_safe_print(e.output)
            return common.RET_ERROR

        # Reset the board
        if type == flashingType.COPROCESSOR:
            command = [programmer['path'] + "\\" + programmer['prg'], "-c",  "port=swd", "sn=" + uart_serial,  "mode=UR",
                    "-ob nSWboot0=1 nboot1=1 nboot0=1", "-rst", "--quietMode", "--verbosity 1"]
        else:
            command = [programmer['path'] + "\\" + programmer['prg'], "-c", "port=SWD", "sn=" + uart_serial, "mode=UR", "-rst",
                        "--quietMode", "--verbosity 1"]
        subprocess.run(command, capture_output=True, text=True)
    else:
        common.thread_safe_print(f"{RED}{header}Flash {displayStr} file {binaryFile} to address {flashAddr} impossible. No available board found.{RESET}")
        return common.RET_ERROR

    return common.RET_OK

def file_exists(file_path):
    if not os.path.isfile(file_path):
        raise argparse.ArgumentTypeError(f"File '{file_path}' does not exist.")
    return file_path

# Main function
def main():
    global programmer
    global external_loader
    global application_firmware_address
    global coprocessor_firmware_address
    global Data_address
    global provisioning_function

    # Parse command line arguments
    parser = argparse.ArgumentParser()

    # Mandatory global option
    parser.add_argument(
        '--provisioning',
        type=int,
        choices=[0, 1],
        required=True,
        help='Provisioning (0 or 1). If 0, only use Json file to populate data factory. If 1 use provisioning script to get certs from third party'
    )

    # Mutually exclusive group for --json and --binary
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '--json',
        metavar="json_file",
        type=file_exists,
        help='Path for Json file containing data to flash in data factory. Flashed at address defined by --flashAddr option. (--flashAddr option has no default value and should be set) (Json file with certs if provisioning is 0, without certs if provisioning is 1. mutually exclusive with --binary)'
    )
    group.add_argument(
        '--binary',
        metavar="binary_file",
        type=file_exists,
        help='Path for binary file containing data to flash in data factory. Flashed at address defined by --flashAddr option. (--flashAddr option has no default value and should be set) (Binary with certs if provisioning is 0, without certs if provisioning is 1. mutually exclusive with --json)'
    )

    # Options for provisioning = 1
    parser.add_argument(
        '--flashCoFW',
        metavar="coprocessor_fw_file",
        type=file_exists,
        help='Coprocessor firmware file path (if provisioning is 1). Flashed at address defined by --CoFwAddr option.  (--CoFwAddr option has no default value and should be set).'
    )
    parser.add_argument(
        '--flashProvisioningFW',
        metavar="provisioning_fw_file",
        type=file_exists,
        help='Application firmware provisioning file path (STM32 or third party) (if provisioning is 1). Flashed at address defined ' +\
             'by --AppFwAddr option (--AppFwAddr  option has no default value and should be set).'
    )
    parser.add_argument(
        '--provisioning_script',
        metavar="provisioning_script",
        type=file_exists,
        help='path to provisioning script to use if provisioning is 1'
    )
    parser.add_argument(
        '--flashAppFW',
        metavar="app_fw_file",
        type=file_exists,
        help='Application matter file path (if provisioning is 1). Flashed at address defined ' +\
             'by --AppFwAddr option (--AppFwAddr  option has no default value and should be set).'
    )
    parser.add_argument(
        '--mode',
        choices=['test', 'production'],
        help='Mode (test(external flash) or production(CKS) when provisioning script allows it) (if provisioning is 1)'
    )

    parser.add_argument("--AppFwAddr", "-afwa",
        metavar="app_fw_address",
        help="Address where the application firmware is flashed. No default value")
    parser.add_argument("--CoFwAddr", "-cofwa",
         metavar="coprocessor_fw_address",
         help="Address where the coprocessor firmware is flashed. No default value")
    parser.add_argument("--flashAddr", "-fa",
        metavar="data_factory_address",
        required=True,
        help="Flash the bin output file to this address. Mandatory")

    #global optional options
    parser.add_argument("--programmerPath", "-pp",
         metavar="stm32CubeProgrammer_path",
         help="Path to STM32CubeProgrammer. If not set, " + programmer['path'] + " will be used.")
    parser.add_argument("--externalLoader", "-el",
        metavar="external_flash_loader_file",
        help="External loader file to use. If not set, " + external_loader['drv'] + " will be used. External loader is in this folder : " +\
            external_loader['path'] + " .")
    parser.add_argument("--keep", "-k", action="store_true", # Do not remove the temporary generated binary file (RnD / debug).
        help=argparse.SUPPRESS)
    parser.add_argument("--verbose", "-v", action="store_true",
        help="Display recognised items when reading json or binary file.")

    args = parser.parse_args()

    # Validate the arguments based on the value of provisioning
    if args.provisioning == 0:
        if not args.json and not args.binary:
            parser.error("--json or --binary with certs is required when provisioning is 0")
    elif args.provisioning == 1:
        if not ((args.json or args.binary) and args.flashProvisioningFW and args.provisioning_script and args.flashAppFW
                and args.mode and args.AppFwAddr and args.flashAddr):
            parser.error("All options (flashCoFW, flashProvisioningFW, provisioning_script, flashAppFW, mode, CoFwAddr, AppFwAddr, flashAddr) " +\
                         "are required when provisioning is 1")
        # Check if args.flashCoFW is set, then args.CoFwAddr must also be set
        if args.flashCoFW and not args.CoFwAddr:
            parser.error("Error: --CoFwAddr must be set if --flashCoFW is specified.")

    coprocessor_firmware_address = args.CoFwAddr
    application_firmware_address = args.AppFwAddr
    Data_address = args.flashAddr

    if args.programmerPath is not None:
        programmer['path'] =  args.programmerPath

    if args.externalLoader is not None:
        external_loader['path'] =  args.externalLoader

    if args.provisioning == 1:
        # Dynamically import the provisioning script and set the provisioning function
        script_path = args.provisioning_script
        spec = importlib.util.spec_from_file_location("provisioning_script", script_path)
        script_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(script_module)
        provisioning_function = getattr(script_module, 'Provisioning')

    uarts = []
    if (args.flashProvisioningFW is not None or
        args.flashCoFW is not None or
        args.flashAppFW is not None or
        args.binary is not None or
        args.json is not None
    ):

        print(f"{BLUE}Get serial port to be used{RESET}")
        # Get the list of available serial ports
        ports = list_serial_ports(programmer)

        # Check if any ports were found
        if ports:
            print(f"{GREEN}Available serial ports :{RESET}")
            for p in ports:
                print(f"  Device : {p['uart_device']} - port : {p['uart_port']} - serial : {p['uart_serial']}")
            for p in ports:
                # Print the port and hardware ID being attempted
                print(f"{BLUE}Check {p['uart_port']} : {p['uart_serial']} ({p['uart_device']}){RESET}")
                # Check if the current port is available for use
                if check_port_availability(p['uart_port'], baud_rate) == True:
                    # If available, set the UART port and indicate that a port has been found
                    uarts.append(
                        {
                            'port' : p['uart_port'],
                            'serial' : p['uart_serial']
                        }
                    )
                    # Current port is available for use
                    print(f"{GREEN}com = {uarts[-1]['port']}, serial = {uarts[-1]['serial']} available. Will be used.{RESET}")
                else:
                    # If the port is not available, print a message indicating it is in use
                    print(f"{RED}The serial port {p['uart_port']} is in use by another application.{RESET}")
        else:
            print(f"{RED}No serial ports found.{RESET}")
        if len(uarts) == 0:
            print(f"{RED}No serial ports available.{RESET}")
            print(f"{RED}Serial port is needed for requested command. Exit.{RESET}")
            exit(-1)

    if len(uarts) == 0:
        globalProcess(args, None)
    else:
        # Create a ThreadPoolExecutor
        with concurrent.futures.ThreadPoolExecutor() as executor:
            # Submit tasks to the executor
            futures = [executor.submit(globalProcess, args, u) for u in uarts]

            # Wait for all tasks to complete
            for future in concurrent.futures.as_completed(futures):
                result, status = future.result()
                if status == common.RET_OK:
                    print(f"{YELLOW}---Task {result} completed successfully---{RESET}")
                else:
                    print(f"{MAGENTA}---Task {result} completed with error---{RESET}")

def globalProcess(args, uart):
    # Dictionary to store TLVs
    data_dict = {}
    header=""
    BinaryFileOutput = ""

    if uart is not None:
        toReturn = uart['port'] + " with serial " + uart['serial']
    else:
        toReturn = None

    if uart is not None:
        common.thread_safe_print(f"{GREEN}---Task {uart['port']} with serial {uart['serial']} started{RESET}---")
        header=uart['port'] + '/..' + uart['serial'][-4:] + "> "
    if args.flashCoFW is not None:
        common.thread_safe_print(f"{BLUE}{header}Flashing coprocessor firmware {args.flashCoFW} to address {coprocessor_firmware_address} ...{RESET}")
        ret = flashBinary(header, args.flashCoFW, coprocessor_firmware_address, programmer, uart['serial'], external_loader, flashingType.COPROCESSOR)
        if ret == common.RET_ERROR:
            return (toReturn, common.RET_ERROR)

    if args.flashProvisioningFW is not None:
        common.thread_safe_print(f"{BLUE}{header}Flashing application firmware {args.flashProvisioningFW} to address {application_firmware_address} ...{RESET}")
        ret = flashBinary(header, args.flashProvisioningFW, application_firmware_address, programmer, uart['serial'], external_loader, flashingType.APPLICATION)
        if ret == common.RET_ERROR:
            return (toReturn, common.RET_ERROR)

    if args.json is not None:
        common.thread_safe_print(f"{BLUE}{header}Read from Json file {args.json}...{RESET}")
        # Read JSON file and fill data_dict
        retValue = read_json_file(header, args.json, data_dict, args.verbose)
        if retValue is not None:
            common.thread_safe_print(f"{GREEN}{header}Read from Json file {args.json} successful{RESET}")
            data_dict = retValue
            BinaryFileOutput = replace_extension_with(args.json, uart['port'], "bin")

    if args.binary is not None:
        common.thread_safe_print(f"{BLUE}{header}Read from binary file {args.binary}...{RESET}")
        retValue = readBinary(header, args.binary, data_dict, args.verbose)
        if retValue is not None:
            common.thread_safe_print(f"{GREEN}{header}Read from binary file {args.binary} successful{RESET}")
            data_dict = retValue
            if not BinaryFileOutput:
                BinaryFileOutput = replace_extension_with(args.binary, uart['port'], "bin")

    # Sort data_dict by ID
    data_dict = dict(sorted(data_dict.items()))

    # if data was read
    if BinaryFileOutput:
        common.thread_safe_print(f"{BLUE}{header}Write to binary file {BinaryFileOutput}...{RESET}")
        # Write data_dict to JSON file
        retValue = writeBinary(header, data_dict, BinaryFileOutput)

        if retValue == 0:
            # Print a message indicating that the binary file is being flashed to the specified address
            common.thread_safe_print(f"{BLUE}{header}Flashing data factory file {BinaryFileOutput} to address {Data_address} ...{RESET}")
            ret = flashBinary(header, BinaryFileOutput, Data_address, programmer, uart['serial'], external_loader, flashingType.DATAFACTORY)
            if ret == common.RET_ERROR:
                return (toReturn, common.RET_ERROR)
        else:
            common.thread_safe_print(f"{RED}{header}Flashing data factory file {BinaryFileOutput} to address {Data_address} Impossible "+\
                f"as binary data factory file can't be written{RESET}")
        if not args.keep and os.path.exists(BinaryFileOutput):
            os.remove(BinaryFileOutput)

    if args.provisioning == 1:
        # Check if the com port is specified
        if uart['port'] is not None:
            common.thread_safe_print(f"{BLUE}{header}Use Com port {uart['port']} for provisioning{RESET}")
            # Call the provisioning function
            ret, error = provisioning_function(1 if args.mode == "production" else 0,
                                               header,
                                               uart['port'])

            common.thread_safe_print(ret)
            if error == common.RET_ERROR:
                return (toReturn, common.RET_ERROR)
        else:
            common.thread_safe_print(f"{RED}{header}Provisioning not possible : no usable com port found.{RESET}")
            return (toReturn, common.RET_ERROR)

    if args.flashAppFW is not None:
        common.thread_safe_print(f"{BLUE}{header}Flashing application firmware {args.flashAppFW} to address {application_firmware_address} ...{RESET}")
        ret = flashBinary(header, args.flashAppFW, application_firmware_address, programmer, uart['serial'], external_loader, flashingType.APPLICATION)
        if ret == common.RET_ERROR:
            return (toReturn, common.RET_ERROR)

    return (toReturn, common.RET_OK)


if __name__ == "__main__":
    main()