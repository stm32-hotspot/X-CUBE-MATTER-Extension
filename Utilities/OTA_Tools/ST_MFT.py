#!/usr/bin/env python3

"""
    @file    ST_MFT.py
    @author  Matter Team
    @brief   ST Matter Firmware Tool
             Graphic User Interface for various scripts to build the Matter 
             firmware OTA for the STM32WB55 or STM32WBA6.

    This script provides a Graphic User Interface for various scripts:
        > CreateMatterBin.py
        > ota_image_tool.py

    note: PySimpleGUI library is required.
"""

import os

# === import third party libraries ===
import PySimpleGUI as sg

# === import chiptool libraries ===
from ota_image_tool import validate_header_attributes as OTA_validate_header_attributes
from ota_image_tool import generate_image as OTA_generate_image

# === import custom libraries ===
from ST_ota_image_tool import HeaderAttributes
from ST_ota_image_tool import extract_data_from_chip_config
from CreateMatterBin import parse_input_args as CMB_parse_input_args
from CreateMatterBin import make_bin as CMB_make_bin

# === general settings ===
APPLICATION_NAME = "Matter Firmware Tool"
APP_VERSION_STRING = "v2.0"

DEFAULT_OUTPUT_MATTER_BIN = "MatterIntermediateBinary.bin"
DEFAULT_OTA_IMAGE = "Matter-update-fw.ota"
CURRENT_FOLDER = os.getcwd()

#################################################################
# application class
# This the application class which creates the application window(s)
# It contains the PySimpleGUI events loop which gather events.
#################################################################
class application():

    def __init__(self, bin1_path='', bin2_path='', output_bin=''):

        self.output_bin = output_bin
        self.chip_config_file = None

        VENDOR_ID = 0
        PRODUCT_ID = 0
        VERSION = 0
        VERSION_STR = "0.0"
        DIGEST_ALGORITHM = "sha256"
        MIN_VERSION = None
        MAX_VERSION = None
        RELEASE_NOTES = None
        INPUT_FILES = [DEFAULT_OUTPUT_MATTER_BIN]
        OUTPUT_FILE = DEFAULT_OTA_IMAGE

        self.ota_args = HeaderAttributes(
            INPUT_FILES, OUTPUT_FILE,
            VENDOR_ID,
            PRODUCT_ID,
            VERSION,
            VERSION_STR,
            DIGEST_ALGORITHM, 
            MIN_VERSION, MAX_VERSION, RELEASE_NOTES)        

        # === define application sections ===
        section_Matter_Bin = [[
                    #  Create Matter binary parameters
                    [sg.Text('Create Matter binary', text_color='yellow', background_color=None, font=('Any 16'))],            
                    [sg.Text('Select the binaries to assemble: ')],
                    # Select Binary1
                    [sg.Text('Binary 1 (sfb or bin): '),
                    sg.Input(bin1_path, size=(50, 1), key='-FILENAME_BIN1-'),
                    sg.FileBrowse(initial_folder=CURRENT_FOLDER, 
                                file_types=(("sfb Files", "*.sfb"),("bin Files", "*.bin"),("all Files", "*.*"),))],
                    # Select Binary2                     
                    [sg.Text('Binary 2 (bin): '),
                    sg.Input(bin2_path, size=(50, 1), key='-FILENAME_BIN2-'),             
                    sg.FileBrowse(initial_folder=CURRENT_FOLDER, 
                                file_types=(("bin Files", "*.bin"),("all Files", "*.*"),))],
                    # Output binary name
                    [sg.Text('Output binary name: '),
                    sg.Input(self.output_bin, size=(50, 1), enable_events=True, key='-FILENAME_OUT_BIN-')],     
                    # Buttons row for CreateMatterBin             
                    [sg.Button('Create MATTER bin', key="-BUTTON_MATTER_BIN-", button_color=('white', 'red'))],
                    [sg.HorizontalSeparator()],                                                         
                    ]]
                
        section_OTA_Fw = [[
                    # OTA image parameters 
                    [sg.Text('OTA image generation', text_color='yellow', background_color=None, font=('Any 16'))],
                    [sg.Text('parameters to extract from CHIPProjectConfig.h')],
                    [sg.Input(self.chip_config_file, size=(50, 1), key='-FILENAME_CHIP_CONFIG-'),
                    sg.FileBrowse(initial_folder=CURRENT_FOLDER, 
                                file_types=(("header Files", "*.h"),("all Files", "*.*"),))],  
                    [sg.Button('Extract data', key="-BUTTON_EXTRACT_DATA-")],                                     
                    [sg.Text('VENDOR_ID: '),
                    sg.Input(hex(self.ota_args.vendor_id), size=(50, 1), key='-VENDOR_ID-')],  
                    [sg.Text('PRODUCT_ID: '),
                    sg.Input(hex(self.ota_args.product_id), size=(50, 1), key='-PRODUCT_ID-')],    
                    [sg.Text('VERSION: '),
                    sg.Input(self.ota_args.version, size=(10, 1), key='-VERSION-'),
                    sg.Text('VERSION_STRING: '),
                    sg.Input(self.ota_args.version_str, size=(20, 1), key='-VERSION_STR-')],  
                    # OTA image name 
                    [sg.Text('Input binary name: '),
                     sg.Text(self.output_bin, size=(50, 1), key='-FILENAME_INPUT_BIN-')],  
                    [sg.Text('Ouput OTA image: '),
                    sg.Input(self.ota_args.output_file, size=(50, 1), key='-FILENAME_OTA_IMAGE-')],                
                    # Bottom buttons row for OTA image
                    [sg.Button('Create OTA image', key="-BUTTON_OTA_IMAGE-", button_color=('white', 'red'))],
                    [sg.HorizontalSeparator()],                                                            
                    ]]
        

        # === application layout ===
        layout = [
            # Create Matter Binary section
            section_Matter_Bin,

            # Create OTA firmware section
            section_OTA_Fw,

            # Output window (all prints are rerouted to -OUTPUT_WINDOW-)                      
            [sg.Output(size=(50,10), expand_x=True, key='-OUTPUT_WINDOW-')],

            # Bottom buttons row    
            [sg.B('Clear fields', key="-BUTTON_CLEAR-"), 
             sg.Button('Exit')],
            [sg.Text("(STMicroelectronics - 2025)" )]
        ]

        # create app window         
        self.window = sg.Window(APPLICATION_NAME + " - " + APP_VERSION_STRING, layout)

        self.initialized = True         
    # end of function __init
        
    def run(self):
        # --------------------- EVENT LOOP ---------------------
        running = True
        while running:
            event, values = self.window.read()
            running = self.check_event_app(event, values)

        # --------------------- EXIT APPLICATION ---------------------
        # close the app    
        self.closeApp()     
    # end of function run
                
    def closeApp(self):
        """ 
        close properly the application
        add clean operations here if needed...
        """
        pass
    # end of function closeApp

    def check_event_app(self, event, values):
        running = True

        #print("event: ", event)
        #print("values: ", values)
        if event == sg.WIN_CLOSED or event == 'Exit':
            running = False     

        # Create Matter bin (header + M4 bin + M0 bin)
        elif event == '-BUTTON_MATTER_BIN-':   
            print("-=-=-=-=-=-=-=-=-=-=-")
            ret = CMB_make_bin(values['-FILENAME_BIN1-'], 
                               values['-FILENAME_BIN2-'], 
                               values['-FILENAME_OUT_BIN-'])  
            if ret != 0:
                print("Matter binary creation failed.")
            else:
                print("Matter binary created successfully.")

        # Extract infos from CHIP config file
        elif event == '-BUTTON_EXTRACT_DATA-':  
            print("-=-=-=-=-=-=-=-=-=-=-")  
            print(f"extract data from {values['-FILENAME_CHIP_CONFIG-']}")   
            # get data from file
            vendor_id, product_id, sw_version = extract_data_from_chip_config(values['-FILENAME_CHIP_CONFIG-'])
            sw_version_string = "INVALID_VALUE"
            # update GUI
            self.window['-VENDOR_ID-'].update(vendor_id)
            self.window['-PRODUCT_ID-'].update(product_id)
            self.window['-VERSION-'].update(sw_version)
            self.window['-VERSION_STR-'].update(sw_version_string)     

        # Create OTA firmware image
        elif event == '-BUTTON_OTA_IMAGE-':
            print("-=-=-=-=-=-=-=-=-=-=-")
            # update self.ota_args parameters
            self.ota_args.vendor_id = int(values['-VENDOR_ID-'], 16)
            self.ota_args.product_id = int(values['-PRODUCT_ID-'], 16)
            self.ota_args.version = int(values['-VERSION-'])
            self.ota_args.version_str = values['-VERSION_STR-']
            # update files name
            self.ota_args.input_files = [values['-FILENAME_OUT_BIN-']]
            self.ota_args.output_file = values['-FILENAME_OTA_IMAGE-']

            #self.ota_args.print()

            # call CHIPTOOL functions
            OTA_validate_header_attributes(self.ota_args)    
            print("OTA: validate_header_attributes done.")
            OTA_generate_image(self.ota_args)                  
            print("OTA: generate_image done.")                       

        elif event == '-BUTTON_CLEAR-':
            # Clear Binary1
            sg.user_settings_set_entry('-filenames_Bin1-', [])
            sg.user_settings_set_entry('-last filename_Bin1-', '')
            self.window['-FILENAME_BIN1-'].update('')
            # Clear Binary2
            sg.user_settings_set_entry('-filenames_Bin2-', [])
            sg.user_settings_set_entry('-last filename_Bin2-', '')
            self.window['-FILENAME_BIN2-'].update('')
            # clear CHIP config file
            self.window['-FILENAME_CHIP_CONFIG-'].update('')
            # Clear OTA parameters
            self.window['-VENDOR_ID-'].update('0')
            self.window['-PRODUCT_ID-'].update('0')
            self.window['-VERSION-'].update('0')
            self.window['-VERSION_STR-'].update('0.0')
            # Clear output window
            self.window['-OUTPUT_WINDOW-'].update('')

        elif event == "-FILENAME_OUT_BIN-":
            # update text if -FILENAME_OUT_BIN- updated
            self.window['-FILENAME_INPUT_BIN-'].update(values['-FILENAME_OUT_BIN-'])            

        return running        
    # end of function  def check_event_app()

# end of class application()
 

# #################################################################
#  main function
# #################################################################
if __name__ == "__main__":

    # arguments parsing
    bin1_path, bin2_path, output_bin = CMB_parse_input_args()

    if output_bin is None:
        output_bin = DEFAULT_OUTPUT_MATTER_BIN

    # start application
    app = application(bin1_path, bin2_path, output_bin)
    app.run()
# end of main function

