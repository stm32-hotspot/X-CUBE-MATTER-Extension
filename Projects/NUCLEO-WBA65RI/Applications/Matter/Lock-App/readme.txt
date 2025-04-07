Lock-App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
It can be commissioned into an existing Matter Fabric and can interact with other devices within this Fabric.

See Wiki articles (https://wiki.st.com/stm32mcu/wiki/Category:Matter) to demonstrate the features.

This example has been tested with an STMicroelectronics NUCLEO-WBA65RI board 
    
In order to make the program work, you must do the following: 
 - Connect NUCLEO-WBA65RI boards to your PC 
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
After commissioning, it might be necessary to restart the OTBR or the lock device, to update the settings before trying to control the lock device.

****  BUTTON SUMMARY ****
Button Mapping:

- Button 1:
   Press the button for at least 5 seconds to trigger a factory reset.
   Press the button (short press) to save persistent data in non-volatile memory before resetting or removing power supply of the board.
- Button 3:
   Press the button to lock / unlock.

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
 


 
	   
