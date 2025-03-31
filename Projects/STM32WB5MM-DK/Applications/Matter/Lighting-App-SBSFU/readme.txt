****
  @page Lighting-App-SBSFU

  @verbatim
  ********************************************************************************************
  * @file    Matter/Lighting-App-SBSFU
  * @author  MCD Application Team
  * @brief   Description of the Lighting App SBSFU application for Matter OTA Framework
  ********************************************************************************************
  *
  * Copyright (c) 2019-2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *********************************************************************************************
  @endverbatim

@par Application Description 

Lighting-App-SBSFU is the same as Lighting-App but is used in SBSFU environment.
This allows Secure Boot and Secure Firmware Update using Matter OTA procedure or BLE.
So it is possible to securely update M0 and/or M4 binary(ies)
Refer to Lighting-App for functional description.

For this application it is requested to have:

- 1 STM32WB5MM-DK board loaded with: 
    - Wireless coprocessor CM0: stm32wb5x_BLE_Thread_light_dynamic_fw.bin
    - Application CM4: SBSFU_Lighting-App-SBSFU.bin
    
- 1 RPi with OTA Provider and Test Harness image (CSA). 

   
                Device 1                                           Device 2       
             -------------                                      ---------------
             |           |                                  	|             |
     B3    =>|End Device | -----.ota file --------------------> |    RPi      | 
   (Reset)   |           |                                      |     +       |
             |           |                                      |             |
             |           |                                      |Test Harness |
             |           |                                      |     +       |
             |   OTA     |                                      |             |
             | Requestor |<-----BDX Messages ---------------->  |    OTA      |
             |           |                                      |  Provider   |   
             |           |                                      |             |
             -------------                                       -------------

To setup the application :

   a)  Open all projects and build them in the following order: BLE_ota, Secure Engine, SBSFU then Lighting-App-SBSFU.

   b)  Load your stm32wb5x_BLE_Thread_light_dynamic_fw.bin for M0 core on your STM32WB devices. 
  
   c)  Load the Decryption Key which is located under Matter\2_Images_SECoreBin\Binary\OEM_KEY_COMPANY1_key_AES_CBC.bin. The device must be provisioned with a decryption key required by SBSFU, using STM32CubeProgrammer. 

   d)  Load your generated SBSFU_Lighting-App-SBSFU.bin application for M4 core on your STM32WB devices. 

   e) The principal goal of OTA is change the actually software version "n" running on device with a new version "n+1". The following step is required to generate .ota file:
        1- Change software version in Projects\STM32WB5MM-DK\Applications\Matter\Lighting-App-SBSFU\Core\Inc\CHIPProjectConfig.h
        2- Get M4 application .sfb binary : Projects\STM32WB5MM-DK\Applications\Matter\Lighting-App-SBSFU\Binary\Lighting-App-SBSFU.sfb
        3- Use CreateMatterBin.py to concatenate M0 and M4 binaries and generate the final bin for update provider.
           => python CreateMatterBin.py -m4 Lighting-App-SBSFU.sfb -m0 stm32wb5x_BLE_Thread_light_dynamic_fw.bin
        4- Use /connectedhomeip/src/app/ota_image_tool.py (from CSA) to generate MatterM4M0.ota file from this specific binary:
           => /connectedhomeip/src/app/ota_imagetool.py : 
	      ./ota_image_tool.py create -v VENDOR_ID -p PRODUCT_ID -vn SOFTWARE_VERSION(newer or higher numerical(int)) -vs SOFTWARE_STRING_VERSION -da sha256 M4M0-fw.bin M4M0-fw.ota
   
   f) The OTA Provider server, responding to the request by installing the new firmware. To prepare image for Raspberrypi or Linux, get the Test harness Raspberrypi Image (CSA), compile chiptool + OTA Provider and flash bootable Test Harness image to Raspberrypi   
   
   g) To start OTA Test using Raspberrypi with Test Harness image, the following step is required:
         - On Terminal_A start OTBR : 
	    sudo ifconfig wlan0 down (optional to ease Thread network)
	    cd ~/chip-certification-tool/scripts/OTBR
	    sudo ./otbr_start.sh
	    cd ~/apps
         - On Terminal_B : 
	    sudo ./chip-ota-provider-app -f M4M0-fw.ota
         - On Terminal_A : 
	    sudo ./chip-tool pairing onnetwork 1 20202021 
         - On Terminal_A:    
            Extract Thread network with : sudo docker exec -t otbr-chip ot-ctl dataset active -x
	    sudo ./chip-tool pairing ble-thread 3 hex:"Thread network dataset" 20202021 3840
         - On Terminal_A :
	    sudo ./chip-tool otasoftwareupdaterequestor write default-otaproviders '[{"fabricIndex":1, "providerNodeID":1,"endpoint":0}]' 3 0
         - On Terminal_A : 
	    sudo ./chip-tool accesscontrol write acl '[{"fabricIndex":1, "privilege":5, "authMode":2, "subjects":[112233], "targets":null},{"fabricIndex":1,"privilege":3,"authMode":2,"subjects":null,"targets":null}]' 1 0
         - On Terminal_A :
	    sudo ./chip-tool otasoftwareupdaterequestor announce-otaprovider 1 0 0 0 3 0
	    (sudo ./chip-tool otasoftwareupdaterequestor announce-otaprovider 1 0 1 0 3 0)
         - On Terminal_B : See results (BDX transfer). Save provider information in NVM by pushing button B1 on Endevice at start of BDX transfer

To run the application :

  a)  Start the first board by pressing on the reset (B3) button. 
      
  b)  At this stage, the OTA server can initiate the steps of firmware update over Matter. The DUT sends a QueryImage command to OTA Provider.
      The SoftwareVersion should match the value reported by the Basic Information Cluster SoftwareVersion attribute of the DUT.
      Verify the ProtocolsSupported lists the BDX Synchronous protocol.

@par Keywords

THREAD, BLE, Matter, SBSFU, OTA

@par Directory contents 

  Using Matter v1.4 SDK.
  
  Files same as Lighting-App + SBSFU files + OTA files  
 
@par Hardware and Software environment

  - This example has been tested with an STMicroelectronics STM32WB5MM-DK board Rev D.
    
@par How to use it ? 

This application requests having the stm32wb5x_BLE_Thread_light_dynamic_fw.bin binary flashed on the Wireless Coprocessor.
If it is not the case, you need to use STM32CubeProgrammer to load the appropriate binary.
All available binaries are located under /Projects/STM32_Copro_Wireless_Binaries directory.
Refer to UM2237 to learn how to use/install STM32CubeProgrammer.
Refer to /Projects/STM32_Copro_Wireless_Binaries/ReleaseNote.html for the detailed procedure to load the Wireless Coprocessor binary. 

In order to make the program work, you must do the following: 

 - Connect STM32WB5MM-DK board to your PC
 - Open STM32CubdeIDE
 - Build all projects (BLE_ota, Secure Engine, SBSFU then Lighting-App-SBSFU) in this order. Two files (Lighting-App-SBSFU.sfb and SBSFU_Lighting-App-SBSFU.bin) are available under binary\ folder into Lighting-App-SBSFU\ directory. 
 - Lighting-App-SBSFU.sfb contains only the application binary to be used to flash new image through OTA. 
 - Using CubeProgrammer: 
   - Flash BLE-Thread firmware (M0) : STM32WB-Matter-BLE-Thread\Projects\STM32WB_Copro_Wireless_Binaries\STM32WB5x\stm32wb5x_BLE_Thread_light_dynamic_fw.bin
   - Flash user key : STM32WB-Matter-BLE-Thread\Projects\STM32WB5MM-DK\Applications\Matter\2_Images_SECoreBin\Binary\OEM_KEY_COMPANY1_key_AES_CBC.bin dans User Key, option simple
   - Flash application software (M4) :  STM32WB-Matter-BLE-Thread\Projects\STM32WB5MM-DK\Applications\Matter\Lighting-App-SBSFU\Binary\SBSFU_Lighting-App-SBSFU.bin
 - To initiate OTA testing:
   - Modify the STM32WB-Matter-BLE-Thread\Projects\STM32WB5MM-DK\Applications\Matter\Lighting-App-SBSFU\Core\Inc\CHIPProjectConfig.h to change software version.
   - Generate MatterM4M0.bin file.
   - Install connectedhomeIP(CSA) environment.
   - Prepare the .ota file using Linux environment. 
   - The.ota file contains M0+M4 or M4 binary which is created with two tools. First one is CreateMatterBin.py used to merge M0 and M4 binaries in a .bin file with ST header. 
   The add of .bin to the Matter header is done by the secand tool /connectedhomeip/src/app/ota_image_tool.py. 
   Then, the result is a .ota file.
   - Get TestHarness Raspberrypi image from CSA and flash it to Raspberrypi.
   - Start OTA Test using Raspberrypi with Test Harness image.

The UART must be configured as follows:
    - BaudRate = 115200 baud  
    - Word Length = 8 Bits 
    - Stop Bit = 1 bit
    - Parity = none
    - Flow control = none
    - Implicit CR in every LF

@par Project generation with CubeMX

This project was generated with CubeMX (6.13.0) and STM32Cube MCU Package for STM32WB Series (1.21.0).

It uses a .extSettings file that allows the inclusion of the MATTER stack, but also STM32 BLE and Thread stacks (due to a CubeMX limitation).
The syntax of the .extsettings file is described in document UM1718 (Rev 46, paragraph 6.4).
The structure of the provided .extsettings file being complex, a Python script allowing to manipulate the content in a readable manner is 
available in the directory Utilities\extSettings.

NOTE:
The linker file STM32WB5MMGHX_FLASH.ld located in the CubeIDE directory was modified after the CubeMX generation to take into account
the size of the Wireless Coprocessor binary.

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
 
