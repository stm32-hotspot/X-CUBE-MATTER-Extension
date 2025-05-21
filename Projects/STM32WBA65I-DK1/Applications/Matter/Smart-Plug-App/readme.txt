Smart-Plug-App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
It can be commissioned into an existing Matter Fabric and can interact with other devices within this Fabric.

See Wiki articles (https://wiki.st.com/stm32mcu/wiki/Category:Matter) to demonstrate the features.

This example has been tested with an STMicroelectronics STM32WBA65I-DK1 board 
    
In order to make the program work, you must do the following: 
 - Connect STM32WBA65I-DK1 boards to your PC 
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
Then other boot, only Thread is needed.
Simulated power consumption is implemented to demonstrate the Energy and Power measurement clusters.
When the Device is set to On state, measurements are sent to the connected client device.

**** LCD SUMMARY ****
LCD Mapping :
 - The LCD screen displays the commissioning QR code if the device is not commissioned.
 - The LCD screen displays "BLE connected" when the BLE rendezvous started.
 - The LCD screen displays "Network Joined" when the board joined a thread network.
 - The LCD screen displays the plug's state.
 - The LCD screen displays: "Fabric Created": if commissioning success.
                            "Fabric Failed" : if commissioning failed.
                            "Fabric Found"  : if the credentials from the non-volatile memory(NVM) have been found.
****  BUTTON SUMMARY ****
Button Mapping:

- Joystick up:
   Press the button to trigger a factory reset.
- Joystick center:
   Press the button to save persistent data in non-volatile.

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
 


 
	   
