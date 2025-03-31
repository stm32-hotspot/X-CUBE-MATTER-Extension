import serial
import re
import time
import common
from common import RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, RESET

# Function to read lines from the COM port
def read_from_com_port(ser):
    """
    Reads lines from the COM port and checks for a specific pattern.

    Args:
        ser (serial.Serial): The serial port object.

    Returns:
        tuple: A tuple containing an error code and an optional error message.
    """

    # typedef enum
    # {
    #     DATAFACTORY_OK,
    #     DATAFACTORY_DATA_NOT_FOUND,
    #     DATAFACTORY_BUFFER_TOO_SMALL,
    #     DATAFACTORY_PARAM_ERROR,
    #     DATAFACTORY_WRITE_IN_CKS_ERROR,
    #
    # }FACTORYDATA_StatusTypeDef;

    while True:
        try:
            # Read a line from the COM port
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            # Define the pattern to match and capture the three digits
            pattern = r"finish:(\d{3})"

            # Check if the decoded string matches the pattern
            match = re.search(pattern, line)

            if match:
                return int(match.group(1)), None

            # Sleep for a short interval before reading the next line
            time.sleep(0.5)

        except KeyboardInterrupt:
            # Handle user interruption
            # print("Exiting...")
            return 0xFFFF, None

        except Exception as e:
            # Handle any other exceptions
            return 0xFFFE, str(e)

def Provisioning(provision_mode, header, port):
    """
    Manages the provisioning process by reading from the COM port and handling results.

    Args:
        provision_mode (int): The provisioning mode. 0 for TEST, 1 for Production.
        header (str): The header text to display in messages.
        port (str): The COM port to open.

    Returns:
        tuple: A tuple containing an error code and an optional error message.
    """
    # Define the return messages for different error codes
    ReturnedValues = {
        0x0000 : f"{GREEN}{header}OK{RESET}",
        0x0001 : f"{RED}{header}Data not found{RESET}",
        0x0002 : f"{RED}{header}Buffer too small{RESET}",
        0x0003 : f"{RED}{header}Parameter error{RESET}",
        0x0004 : f"{RED}{header}CSK write error{RESET}",
        0xFFFE : f"{RED}{header}Provisioning completed with unknown error{RESET}",
        0xFFFF : f"{RED}{header}Provisioning interrupted by user{RESET}"
    }

    # Print the start message
    common.thread_safe_print(f"{GREEN}{header}Start third party provisioning{RESET}")

    try:
        # Open the COM port
        ser = serial.Serial(port, 115200, timeout=1)
    except Exception as e:
        return (f"{RED}{header}Failed to open COM port: {str(e)}{RESET}", common.RET_ERROR)

    # Call the function to start reading
    result, text = read_from_com_port(ser)

    # Close the COM port
    ser.close()

    # Return the appropriate message based on the result
    if (text is None) and (result == 0X0000):
        return (ReturnedValues.get(result), common.RET_OK)
    if text is None:
        return (ReturnedValues.get(result), common.RET_ERROR)
    else:
        return (f"{RED}{header}{ReturnedValues.get(result)} : {text}{RESET}", common.RET_ERROR)