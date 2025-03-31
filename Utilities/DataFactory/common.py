from datetime import datetime
import threading

# Define ANSI escape codes for text formatting
RED = '\033[91m'       # Red text
GREEN = '\033[92m'     # Green text
YELLOW = "\033[1;33m"  # Yellow text
BLUE = '\033[94m'      # Blue text
MAGENTA = "\033[1;35m" # Magenta text
CYAN = "\033[1;36m"    # Cyan text
RESET = '\033[0m'      # Reset text color to default

RET_OK = 0
RET_ERROR = 1

# Define print lock
print_lock = threading.Lock()

# Returns the current timestamp as a string formatted as 'YYYY-MM-DD HH:MM:SS.ffffff'
def get_current_timestamp():
    # Get the current date and time and Format the current date and time as a string
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")

# Custom print function with lock
def thread_safe_print(*args, **kwargs):
    with print_lock:
        print(f"{get_current_timestamp()} -", *args, **kwargs)
        
def readIntegerFromFile(header, file_path):
    try:
        # Step 1: Open the text file in read mode
        with open(file_path, 'r') as file:
            # Step 2: Read the single line from the file
            line = file.readline().strip()
            
            # Step 3: Convert the line to an integer
            number = int(line)
            
            # Return the integer
            return number

    except FileNotFoundError:
        thread_safe_print(f"{RED}{header}Error: The file '{file_path}' was not found.{RESET}")
    except IOError:
        thread_safe_print(f"{RED}{header}Error: An I/O error occurred while reading the file '{file_path}'.{RESET}")
    except ValueError:
        thread_safe_print(f"{RED}{header}Error: The line in the file '{file_path}' is not a valid integer: '{line}'{RESET}")
    except OSError as e:
        thread_safe_print(f"{RED}{header}Error: An OS error occurred: {e}{RESET}")
    except Exception as e:
        thread_safe_print(f"{RED}{header}An unexpected error occurred: {e}{RESET}")

def readBinaryFileToByteList(header, file_path):
    try:
        # Step 1: Open the binary file in read-binary mode
        with open(file_path, 'rb') as file:
            # Step 2: Read the contents of the file
            binary_data = file.read()

        # Step 3: Convert the contents to a list of bytes
        byte_list = list(binary_data)

        # Return the list of bytes
        return byte_list

    except FileNotFoundError:
        thread_safe_print(f"{RED}{header}Error: The file '{file_path}' was not found.{RESET}")
    except IOError:
        thread_safe_print(f"{RED}{header}Error: An I/O error occurred while reading the file '{file_path}'.{RESET}")
    except OSError as e:
        thread_safe_print(f"{RED}{header}Error: An OS error occurred: {e}{RESET}")
    except Exception as e:
        thread_safe_print(f"{RED}{header}An unexpected error occurred: {e}{RESET}")
