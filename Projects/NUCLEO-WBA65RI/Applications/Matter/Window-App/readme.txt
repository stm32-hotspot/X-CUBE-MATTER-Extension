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
The STM32WBA65I-DK1 board running Window_App application is capable 
of BLE and OpenThread activity at the same time.

@par Keywords

THREAD,BLE,Matter

@par Directory contents 

 
  Using Matter v1.4.0.0 SDK.
  
 
@par Hardware and Software environment

  - This example has been tested with an STMicroelectronics STM32WBA65I-DK1 board 
    
@par How to use it ? 


The libraries used for BLE and OpenThread are provided as part of the project
Refer to release note to identify the STM32CubeIDE and STM32CubeProgrammer versions to be used


Minimum requirements for the demo:
- 1 STM32WBA65I-DK1 board with Window_App firmware.
- 1 OTBR (Raspberry Pi or MP135 + Nucleo RCP) or any supported ecosystem by Matter
- 1 Smartphone (Android) with "CHIPTool" Phone Application (available Utilities/APK/app-debug-v_1_3.zip) 
or chiptool on RPi or Ecosystem application


In order to make the program work, you must do the following: 
 - Connect STM32WBA65I-Discovery boards to your PC 
 - Open STM32CubeIDE
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
Follow the instruction defined in https://wiki.st.com/stm32mcu/wiki/Connectivity:Matter_test_and_demonstrate

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
Joystick Mapping:

- Joystick up:
    save persistent data in non-volatile memory before resetting or removing power supply of the board.
-Joystick Select:
    Press joystick to trigger a factory reset.
 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
 


 
	   
