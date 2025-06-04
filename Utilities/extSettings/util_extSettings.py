#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# script to expand an .extSettings file to readable text files
# and to recombine these files into a .extSettings
# arguments:
# -f [FILE], --file [FILE] : Path to the .extSettings file (optionnal arg, default value = .extSettings)
# -dec, --decode : Expand .extSettings into readable separate files
# -enc, --encode : Combine readable separate files into .extSettings
# 
# The 3 files generated from .extSettings are:
#  > extSettings_ProjectFiles.txt : which contains the include paths
#  > extSettings_Others.txt : which contains the compilation flags
#  > extSettings_Groups.txt : which contains the source file with their path in IDE browser (between [])
#
# The 3 .txt files can be modified before to use --enc
# Comments can be added in text files by starting a line with an exclamation point (!) or a hash mark (#).
# Comments lines will be ignored during files recombination (--enc).
#
# Examples of use:
#  python ./util_extSettings.py --dec
#  python ./util_extSettings.py --end -f new_extSettings
#


import argparse
import os

FILE_INC_PATHS = "extSettings_ProjectFiles.txt"
FILE_DEFINES = "extSettings_Others.txt"
FILE_SRC = "extSettings_Groups.txt"

def extract_section_lines(file_content, section_name):
    section_start = f"[{section_name}]"
    section_lines = []
    capture = False
    for line in file_content.splitlines():
        if line.strip() == section_start:
            capture = True
            continue
        if capture:
            if line.startswith("[") and line.endswith("]"):
                break
            section_lines.append(line)
    return section_lines

def reformat_line(line, section_name):
    _, fields = line.split("=", 1)
    return f"[{section_name}]\n" + "\n".join(fields.split(";"))

def reformat_groups_line(line):
    path, files = line.split("=", 1)
    formatted_lines = f"[{path.strip()}]\n" + "\n".join(files.split(";")) + "\n\n"
    return formatted_lines

def save_to_file(file_name, content):
    with open(file_name, 'w') as file:
        file.write(content)

def process_extSettings(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # Process [ProjectFiles] section
    project_files_lines = extract_section_lines(content, "ProjectFiles")
    for line in project_files_lines:
        if line.startswith("HeaderPath="):
            formatted_content = reformat_line(line, "HeaderPath")
            save_to_file(FILE_INC_PATHS, formatted_content)
            break  # Assuming there's only one HeaderPath line per section

    # Process [Others] section
    others_lines = extract_section_lines(content, "Others")
    for line in others_lines:
        if line.startswith("Define="):
            formatted_content = reformat_line(line, "Define")
            save_to_file(FILE_DEFINES, formatted_content)
            break  # Assuming there's only one Define line per section

    # Process [Groups] section
    groups_lines = extract_section_lines(content, "Groups")
    groups_content = ""
    for line in groups_lines:
        if "=" in line:
            groups_content += reformat_groups_line(line)
    save_to_file(FILE_SRC, groups_content)

def read_file_filtered(file_path):
    with open(file_path, 'r') as file:
        return [line.strip() for line in file if line.strip() and not line.startswith(('#', '!'))]

def encode_to_extSettings(project_files_path, others_path, groups_path, output_path):
    # Read the content of the files, filtering out empty lines and comments
    project_files_content = read_file_filtered(project_files_path)
    others_content = read_file_filtered(others_path)
    groups_content = read_file_filtered(groups_path)

    # Open the output file for writing
    with open(output_path, 'w') as output_file:
        # write the [ProjectFiles] section
        output_file.write("[ProjectFiles]\n")
        if project_files_content:
            output_file.write("HeaderPath=" + ";".join(project_files_content[1:]) + ";\n\n")
        
        # write the [Others] section
        output_file.write("[Others]\n")
        if others_content:
            output_file.write("Define=" + ";".join(others_content[1:]) + ";\n\n")
        
        # write the [Groups] section
        output_file.write("[Groups]")
        for line in groups_content:
            if line.startswith('['):
                path = line.strip("[]")
                output_file.write(f"\n{path}=")
            else:
                # Add each file followed by a semicolon and a newline
                output_file.write(line + ";")
        output_file.write("\n")

def main():
    parser = argparse.ArgumentParser(description="Process .extSettings file")
    
    # Optional argument -f with a default value
    parser.add_argument('-f', '--file', nargs='?', default='.extSettings',
                        help="Path to the .extSettings file")
    
    # Arguments -dec and -enc unchanged because they do not take parameters
    parser.add_argument('-dec', '--decode', action='store_true', help="Expand .extSettings into readable separate files")
    parser.add_argument('-enc', '--encode', action='store_true', help="Combine readable separate files into .extSettings")

    args = parser.parse_args()

    # Use args.file to get the file name, whether it's provided or default
    if args.decode:
        process_extSettings(args.file)
    elif args.encode:
        encode_to_extSettings(FILE_INC_PATHS, FILE_DEFINES, FILE_SRC, args.file)
    else:
        parser.print_help()


# #################################################################
#  main function
# #################################################################

if __name__ == "__main__":
    main()
    print("\ndone.\n")    