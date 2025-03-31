## <b>OEMiROT_Appli_TrustZone Application Description</b>
  ******************************************************************************
  *
  * Copyright (c) 2019-2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  
This project provides an OEMiROT boot path for MATTER.
MATTER MCUboot configuration:
  - Primary and secondary slots are used.
  - "Overwrite" method (not "swap") is usedd to save 64Kb.
  - Secure and Non-*Secure Data slots are not used.
The external loader is disabled (MCUBOOT_EXT_LOADER).

Boot is performed through OEMiROT boot path after authenticity and the integrity checks of the project firmware and project data
images.

This project is composed of two sub-projects:

- One for the secure application part (Project_s)

- One for the non-secure application part (Project_ns) : MATTER Ligthing-App


Please remember that on system with security enabled, the system always boots in secure and
the secure application is responsible for launching the non-secure application. When the secure application is started the MPU
is already configured (by OEMiROT) to limit the execution area to the project firmware execution slot. This is done in order to avoid
any malicious code execution from an unauthorised area (RAM, out of execution slot in user flash ...). Once started, it is up to the secure
application to adapt the security configuration to its needs. In this example, the MPU is simply disabled.

The non-secure application is the MATTER Lighting-App used in the OEMiROT environment.
This allows Secure Boot and Secure Firmware Update using Matter OTA procedure.
Refer to Lighting-App for functional description.

### <b>Memory mapping</b>

start address | size        | size (KB) | content
--------------+-------------+-----------+---------------------------------------------
0x0800.0000   | 0x0000.2000 | 8kb       | "HASH REF"
--------------+-------------+-----------+---------------------------------------------
0x0800.2000   | 0x0000.2000 | 8kb       | "BL2NVCNT"
--------------+-------------+-----------+---------------------------------------------
0X0800.4000   | 0x0000.2000 | 8kb       | "integrated perso data"
--------------+-------------+-----------+---------------------------------------------
0X0800.6000   | 0x0001.0000 | 64kb      | "OEMIROT BOOT"
--------------+-------------+-----------+---------------------------------------------
0X0801.6000   | 0x0000.2000 | 8kb       | "HPD activation code"
--------------+-------------+-----------+---------------------------------------------
0X0801.8000   | 0x0000.6000 | 24kb      | "Secure Image Primary Slot (Area 0)"
--------------+-------------+-----------+---------------------------------------------
0X0801.E000   | 0x000E.0000 | 896kb     | "Non-Secure Image Primary Slot (Area 1)"
--------------+-------------+-----------+---------------------------------------------
0X080F.E000   | 0x0000.6000 | 24kb      | "Secure Image Secondary Slot (Area 2)"
--------------+-------------+-----------+---------------------------------------------
0X0810.4000   | 0x000E.0000 | 896kb     | "Non-Secure Image Secondary Slot (Area 3)"
--------------+-------------+-----------+---------------------------------------------
0X081E.4000   | 0x0000.8000 | 32kb      | "NVM"
--------------+-------------+-----------+---------------------------------------------
0X081E.C000   | 0x0000.F000 | 60kb      | "DATA FACTORY"
--------------+-------------+-----------+---------------------------------------------
0X081F.B000   | 0x0000.3000 | 12kb      | "free space"
--------------+-------------+-----------+---------------------------------------------
0X081F.E000   | 0x0000.2000 | 8kb       | "reserved"
--------------+-------------+-----------+---------------------------------------------

### <b>Keywords</b>

TrustZone, OEMiROT, Boot path, Root Of Trust, Security, MPU, MATTER, Lighting

### <b>Directory contents</b>

  - OEMiROT_Appli_TrustZone/Secure_nsclib/appli_flash_layout.h          Flash mapping for appli NS & S
  - OEMiROT_Appli_TrustZone/Secure_nsclib/appli_region_defs.h           RAM and FLASH regions definitions for appli NS & S

  - OEMiROT_Appli_TrustZone/Secure/Inc/low_level_flash.h                Header file for low_level_flash.c
  - OEMiROT_Appli_TrustZone/Secure/Inc/low_level_spi_flash.h            Header file for low_level_spi_flash.c
  - OEMiROT_Appli_TrustZone/Secure/Inc/m95p32_conf.h                    Header file for BSP spi flash configuration
  - OEMiROT_Appli_TrustZone/Secure/Inc/main.h                           Header file for main.c
  - OEMiROT_Appli_TrustZone/Secure/Inc/partition_stm32wbaxx.h           STM32WBAxx Device System Configuration file
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32_board.h                    Definitions for stm32wbaxx board
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32_hal.h                      Header file for wba specifics include
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32wbaxx_hal_conf.h            HAL configuration file
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32wba55g_discovery_conf.h     STM32WBA55G Discovery board configuration file
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32wba55g_discovery_errno.h    STM32WBA55G Discovery board errors file
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32wba65i_discovery_conf.h     STM32WBA65I Discovery board configuration file
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32wba65i_discovery_errno.h    STM32WBA65I Discovery board errors file
  - OEMiROT_Appli_TrustZone/Secure/Inc/stm32wbaxx_it.h                  Header file for stm32wbaxx_it.c

  - OEMiROT_Appli_TrustZone/Secure/Src/low_level_device.c               Flash Low level device setting
  - OEMiROT_Appli_TrustZone/Secure/Src/low_level_flash.c                Flash Low level interface
  - OEMiROT_Appli_TrustZone/Secure/Src/low_level_spi_device.c           External Flash Low level device configuration
  - OEMiROT_Appli_TrustZone/Secure/Src/low_level_spi_flash.c            External Flash Low level interface
  - OEMiROT_Appli_TrustZone/Secure/Src/main.c                           Secure Main program
  - OEMiROT_Appli_TrustZone/Secure/Src/secure_nsc.c                     Secure Non-secure callable functions
  - OEMiROT_Appli_TrustZone/Secure/Src/startup_stm32wbaxx.c             Startup file in c
  - OEMiROT_Appli_TrustZone/Secure/Src/stm32wbaxx_hal_msp.c             Secure HAL MSP module
  - OEMiROT_Appli_TrustZone/Secure/Src/stm32wbaxx_it.c                  Secure Interrupt handlers
  - OEMiROT_Appli_TrustZone/Secure/Src/system_stm32wbaxx.c              Secure STM32WBAxx system clock configuration file

  - OEMiROT_Appli_TrustZone/NonSecure                                   The MATTER Ligthing-App

### <b>Hardware and Software environment</b>

  - This example runs on STM32WBA65I-DK1 devices with security enabled (TZEN=B4).
  - This example has been tested with STMicroelectronics STM32WBA65I-DK1 (MB2130)
  - To print the application menu in your UART console you have to configure it using these parameters:
    Speed: 115200, Data: 8bits, Parity: None, stop bits: 1, Flow control: none.

### <b>How to use it ?</b>

<u>Before compiling the project, you should first start the provisioning process</u>. During the provisioning process, the linker files
will be automatically updated.

Before starting the provisioning process, select the application project to use (application example or template),
through oemirot_appli_path_project variable in ROT_Provisioning/env.bat or env.sh.
Then start provisioning process by running provisioning.bat (.sh) from ROT_Provisioning/OEMiROT or ROT_Provisioning/OEMiROT_OEMuROT,
and follow its instructions. Refer to ROT_Provisioning/OEMiROT or ROT_Provisioning/OEMiROT_OEMuROT readme for more information on
the provisioning process for OEMiROT boot path or OEMiROT_OEMuROT boot path.

Steps to follow:

1- Import project OEMiROT_Boot\STM32CubeIDE in STM32CubeIDE and compile.

2- Import project OEMiROT_Appli_TrustZone\STM32CubeIDE in STM32CubeIDE. 
  It consists of 2 sub-projects: the Secure application and the Non-Secure application.
  Compile Non-Secure application project (it will automatically compile Secure application project).

  Binaries are generated in OEMiROT_Appli_TrustZone\Binary:
    - oemirot_tz_ns_app.bin : the clear NSec-app binary.
    - oemirot_tz_ns_app_enc_sign.bin : the signed NSec-app binary, not padded (used for OTA).
    - oemirot_tz_ns_app_init_sign.bin : the signed NSec-app binary, padded (used by provisioning script).
    - oemirot_tz_s_app.bin : the clear Sec-app binary.
    - oemirot_tz_s_app_enc_sign.bin : the signed Sec-app binary, not padded (used for OTA, not supported yet).
    - oemirot_tz_s_app_init_sign.bin : the signed Sec-app binary, padded (used by provisioning script).

3- To Flash the board, go into ROT_Provisioning\OEMiROT directory.
  Plug the USB cable (CN6) and launch the script regression.sh/.bat
  Follow the instruction displayed.
  Then launch the script provisioning.sh/.bat : select RDP 0 and follow instructions (press any key).

  Reboot the board, MATTER Lighting-App should start.

  ***********************
  *** IMPORTANT NOTES ***
  ***********************
  Note 1: at the time of writing, only .sh scripts are generated for CubeIDE. Windows cannot natively use .sh scripts, user must use a 
          third-party application (such as Git BASH, WSL, MinGW, Cygwin,…) to launch the scripts.
  Note 2: After the script regression.sh is used, the trustzone will be activated on the STM32WBA65I-DK1 board. If you want to flash a 
          project without OEMiROT, it will be necessary to disable the TrustZone beforehand.
          See Wiki article for more information.
  Note 3: The regression script may fail the very first time it is used on a board without TrustZone activated.
		  If this happens, the following procedure must be followed:
		    - step 1: Open CubeProgrammer (v2.19.0 or +) and set the connect "Mode" to "Hot Plug", then connect to the WBA6 board.
			- step 2: Modify Option Bytes settings as follow, then click to "Apply":
	      		
                                |  SAU1   ||          WRP1           || SAU2    ||          WRP2           |
                                |  SECWM1 || WRP1A      | WRP1B      || SECWM2  || WRP2A      | WRP2B      |
                                |STRT|END ||A-STRT|A-END|B-STRT|B-END||STRT|END ||A-STRT|A-END|B-STRT|B-END|
             +------------------+----+----++------+-----+------+-----++----+----++------+-----+------+-----+
             |Option Byte value |0x0 |0x7F||0x7F  |0x0  |0x7F  |0x0  ||0x7F|0x0 ||0x7F  |0x0  |0x7F  |0x0  |
             +------------------+----+----++------+-----+------+-----++----+----++------+-----+------+-----+

            - step 3: Disconnect the board from CubeProgammer.
			- step 4: Start now the provisioning.sh script (do not restart regression before).
          See Wiki article for more information.

4- Generate an OTA Firmware update image
  The OMEiROT version must be incremented before generating a new image. This is done by updating:
    - In the file ROT_Provisioning\OEMiROT\Images\OEMiROT_NS_Code_Image.xml, update the value of Version parameter.

  The version of MATTER must be incremented before generating a new image. This is done by updating:
    - CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION  in the file OEMiROT_Appli_TrustZone\NonSecure\Core\Inc\CHIPProjectConfig.h
    - X_CUBE_MATTER_VERSION string in the file OEMiROT_Appli_TrustZone\NonSecure\Core\Inc\app_entry.h

  Finally, use the file OEMiROT_Appli_TrustZone\Binary\oemirot_tz_ns_app_enc_sign.bin with the scripts in \Utilities\OTA_Tools
  See readme.txt for more information.
  At the end, you will have an .ota file.

For more details, refer to STM32WBA Wiki articles:

  - [STM32WBA security](https://wiki.st.com/stm32mcu/wiki/Category:STM32WBA).
  - [OEMiRoT OEMuRoT for STM32WBA](https://wiki.st.com/stm32mcu/wiki/Security:OEMiRoT_OEMuRoT_for_STM32WBA).
  - [How to start with ROT on STM32WBA](https://wiki.st.com/stm32mcu/wiki/Category:How_to_start_with_RoT_on_WBA).

#### <b>Note:</b>

  1. The most efficient way to develop and debug an application is to boot directly on user flash in RDL level 0 by setting with
     STM32CubeProgrammer the RDP to 0xAA and the SECBOOTADD to (0x0C006000 + offset of the firmware execution slot).

  2. Two versions of ROT_AppliConfig are available: windows executable and python version. By default, the windows executable is selected. It
     is possible to switch to python version by:
        - installing python (Python 3.10 or newer) with the required modules listed in requirements.txt.
        ```
        pip install -r requirements.txt
        ```
        - having python in execution path variable
        - deleting main.exe in Utilities\PC_Software\ROT_AppliConfig\dist
