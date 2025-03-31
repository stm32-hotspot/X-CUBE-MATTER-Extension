==============================================================================
util_extSettings.py
==============================================================================
origin: STMicroelectronics

description: 
This python (v3) script goal is to ease reading/writing of .extsettings files.
Natively, a large .extsettings file is difficult to read or modify.
This script breaks a .extsettings file into 3  easily readable text files.
The 3 files contain respectively:
- the list of included paths
- the list of compiled files
- the list of compilation flags
These files can be modified and then recombined to update the .extsettings file.

NOTE:
If the generation of CubeMX fails, there is no indication which makes it difficult to detect the origin of error.
A syntax error in the .extsettings file may be the cause of this error.
See document UM1718 (Rev 46, paragraph 6.4) for more informations about the .extsettings file.

usage examples:
Help
python ./util_extSettings.py --help

Decode a .extsettings file
python ./util_extSettings.py -dec -f ..\..\Projects\STM32WB5MM-DK\Applications\Matter\Lighting-App\.extSettings

Encode a .extsettings file in current directory
python ./util_extSettings.py -enc

Encode a .extsettings file in a specific directory
python ./util_extSettings.py -enc -f ..\..\Projects\STM32WB5MM-DK\Applications\Matter\Lighting-App\.extSettings
