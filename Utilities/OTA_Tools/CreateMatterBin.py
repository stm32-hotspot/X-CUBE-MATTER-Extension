# -*- coding: utf-8 -*-

"""
    @file    CreateMatterBin.py
    @author  Matter Team
    @brief   Create Matter binary file by assembling header + 1 or 2 binary files.

    Python 3 is required.

    Description:
    The script creates a Matter binary file consisting of:
        - a header (8 bytes) 
        - 1 or 2 binary files

    The header is composed as follows: 
        - the first 4 bytes indicate the size of the first binary file.
        - the last 4 bytes indicate the size of the second binary file.

    How to use:
    The script is compatible with WB55 and WBA6.
    Launch the scripts with following arguments : 
        -bin1 followed by full path to 1st binary (mandatory)
        -bin2 followed by full path to 2nd binary (optional)
        -o followed by output bin (optional)

    For WB55:
        -bin1 corresponds to M4 bin and -bin2 corresponds to M0 bin.
        note: alternatively, the options -m4 and -m0 can be used.

    For WBA6:
        -bin1 corresponds to NonSecureApp bin and -bin2 corresponds to SecureApp bin.
        note: alternatively, the options -nspp and -sapp can be used.
        note: in the current version, the option -sapp is not supported.

    Example WB55:
    python.exe .\CreateMatterBin.py -bin1 c:\MATTER\path_to_M4_bin\my-M4.bin -bin2 c:\MATTER\path_to_M0_bin\my-M0.bin

    Example WBA6:
    python.exe .\CreateMatterBin.py -bin1 c:\MATTER\path_to_NSAPP_bin\my-NSAPP.bin

    If successful, Matter binary file is created in script directory.        
"""

import os
import struct
import argparse

CreateMatterBin_about_name = "CreateMatterBin"
CreateMatterBin_about_version = "v2.0"
CreateMatterBin_about_date = "2025-02-19"

DEFAULT_OUTPUT_MATTER_BIN = "MatterIntermediateBinary.bin"

# #################################################################
#  make_bin function
#  Create Matter binary files made of:
#   - Header
#   - 1st binary file (M4 or NonSecureApp)
#   - 2nd binary file (M0 or SecureApp, optional)
# Return code : 0 if success
# #################################################################
def make_bin(file_path_Bin1, file_path_Bin2, file_path_Matter):

    # Read 1st binary content
    try:
        Binary1_Size = os.path.getsize(file_path_Bin1)
        with open(file_path_Bin1,'rb') as f:
            dataM4 = f.read()
            f.close()
    except:
        print("Error opening 1st binary")
        return(1)    
     
    # Read 2nd binary content  
    if file_path_Bin2:
        try:
            Binary2_Size = os.path.getsize(file_path_Bin2)                
            with open(file_path_Bin2,'rb') as f:
                dataM0 = f.read()
                f.close()
        except:
            print("Error opening 2nd binary")
            return(1)     
    else:
        # if no 2nd binary provided, uses an empty 2nd binary
        Binary2_Size = 0
        dataM0 = b''

    print("*** 1st binary (M4 or NS-App) ***")
    print(file_path_Bin1)    
    print("size: ", Binary1_Size)
    print("*** 2nd binary (M0 or S-App) ***")
    print(file_path_Bin2)    
    print("size: ", Binary2_Size)

    # Create the header
    print("*** MATTER binary ***")
    Header = struct.pack("II", Binary1_Size, Binary2_Size)
    print("Header = ", Header)

    # Write Matter bin content   
    try:        
        with open(file_path_Matter,'wb') as f:
            f.write(Header + dataM4 + dataM0)
            f.close()
            print("Output= ", file_path_Matter)
    except:
        print("Error during binary creation")
        return(1)      

    # success
    return(0) 

# #################################################################
#  parse_input_args function
#  This function parse input arguments
#  Returns: bin1_path, bin2_path
# #################################################################
def parse_input_args():
    # arguments parsing
    parser = argparse.ArgumentParser(description='Parmaters parsing')
    parser.add_argument('-bin1', '-m4', '-nsapp', type=str,
                        help='full path to 1st binary file (M4 or NS-App)')
    parser.add_argument('-bin2', '-m0', '-sapp', type=str,
                        help='full path to 2nd binary file (M0 or S-App)', default=None)
    parser.add_argument('-o', type=str,
                        help='output bin file name')

    args = parser.parse_args()

    return args.bin1, args.bin2, args.o  

# #################################################################
#  main function
# #################################################################
def main():

    print("< create Matter bin started >")
    # parse input arguments
    Bin1_path, Bin2_path, output_bin = parse_input_args()

    if output_bin is None:
        output_bin = DEFAULT_OUTPUT_MATTER_BIN

    if Bin1_path:
        # all mandatory args are present, create the Matter bin
        ret = make_bin(Bin1_path, Bin2_path, output_bin)  
        if ret != 0:
            print("Matter binary creation failed.")
        else:
            print("Matter binary created.")  
    else:
        # missing mandatory args
        print("Error, argument -m4 is mandatory.")

# #################################################################
#  Entry point
# #################################################################
if __name__ == '__main__':

    """ Main starts here """
    main()