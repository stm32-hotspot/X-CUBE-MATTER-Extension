****
  @page Window_App
  @verbatim
  ******************************************************************************
  * @file    Matter/Window_App
  * @author  MCD Application Team
  * @brief   This file lists the modification done by STMicroelectronics on
  *          Matter for integration with STM32Cube solution.
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
  @endverbatim



Window_App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
It can be commissioned into an existing Matter Fabric and can interact with other devices within this Fabric.
The STM32WB5MM-DK board running Window-app application is capable 
of BLE and OpenThread activity at the same time.

See Wiki articles (https://wiki.st.com/stm32mcu/wiki/Category:Matter) to demonstrate the features.

@par Keywords

THREAD,BLE,Matter

@par Directory contents 

  Using Matter v1.4 SDK.
  
  - Core directory           Core STM32 initialization.
  - Matter directory         Matter specific code and Cluster implementation.
  - STM32_WPAN directory     ST BLE and Thread Profile implementation, Matter Profile implementation.
  - System directory         STM32 utilities and system management.
  
 
@par Hardware and Software environment

  - This example has been tested with an STMicroelectronics STM32WB5MM-DK board 
    

@par How to use it ? 

This application requests having the stm32wb5x_BLE_Thread_light_dynamic_fw.bin binary flashed on the Wireless Coprocessor.
If it is not the case, you need to use STM32CubeProgrammer to load the appropriate binary.
All available binaries are located under /Projects/STM32_Copro_Wireless_Binaries directory.
Refer to UM2237 to learn how to use/install STM32CubeProgrammer.
Refer to /Projects/STM32_Copro_Wireless_Binaries/ReleaseNote.html for the detailed procedure to load the proper
Wireless Coprocessor binary. 


Minimum requirements for the demo:
- 1 STM32WB5MM-DK board with Window-App firmware.
- 1 OTBR (Raspberry Pi or MP135 + Nucleo RCP)
- 1 Smartphone (Android) with "CHIPTool" Phone Application (available Utilities/APK/app-debug-v_1_1.zip) or chiptool on OTBR


In order to make the program work, you must do the following: 
 - Connect STM32WB5MM-DK boards to your PC 
 - Open STM32CubdeIDE
 - Rebuild all files and load your image into target memory
 - Run the example
 
To get the traces in real time, you can connect an HyperTerminal to the STLink Virtual Com Port. 
    
 For the the traces, the UART must be configured as follows:
    - BaudRate = 115200 baud  
    - Word Length = 8 Bits 
    - Stop Bit = 1 bit
    - Parity = none
    - Flow control = none

**** START DEMO ****
1st boot the End Device must be commissioned (BLE then Thread)
Then other boot, only Thread is needed

**** LCD SUMMARY ****
LCD Mapping :
 - The LCD screen displays "BLE connected" when the BLE rendezvous started.
 - The LCD screen displays "Network Joined" when the board joined a thread network.
 - The LCD screen displays the opening pourcentage of the windows or the finish status
 - The LCD screen displays: "Fabric Created": if commissioning success.
                            "Fabric Failed" : if commissioning failed.
                            "Fabric Found"  : if the credentials from the non-volatile memory(NVM)
                                              have been found 
****  BUTTON SUMMARY ****
Button Mapping:

- Button 1:
   Press the button for at least 10 seconds to trigger a factory reset.
   Press the button (short press) to save persistent data in non-volatile memory before resetting or removing power supply of the board.

****  PROJECT GENERATION WITH CUBEMX ****
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
 


 
	   
