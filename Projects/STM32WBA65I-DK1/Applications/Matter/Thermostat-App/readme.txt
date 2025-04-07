Thermostat_App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
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
Simulated temperature measurement is done to demonstrate the ability to upload temperature data.
Binding cluster is implemented - the temperature from an external sensor (using the Temperature-Measure-App application) can be propagated to the thermostat device and will be used as "external temperature" as per thermostat specification. Binding is only support by the Chip-tool Android application with OTBR running on a Raspberry Pi.
NVM user data management is implemented. In TemperatureManager.cpp, it is used to store the configured mode and setpoints and so these values are restored after power-reset. The user can extend this to additional data.

**** LCD SUMMARY ****
LCD Mapping :
 - The LCD screen displays the commissioning QR code if the device is not commissioned.
 - The LCD screen displays "BLE connected" when the BLE rendezvous started.
 - The LCD screen displays "Network Joined" when the board joined a thread network.
 - The LCD screen displays the measured temperature, setpoints and mode.
 - The LCD screen displays: "Fabric Created": if commissioning success.
                            "Fabric Failed" : if commissioning failed.
                            "Fabric Found"  : if the credentials from the non-volatile memory(NVM) have been found.
****  BUTTON SUMMARY ****
Button Mapping:

- Joystick up:
   Press the button to trigger a factory reset.
- Joystick center:
   Press the button to save persistent data in non-volatile.
- Joystick right:
   Press the button to increase the temperature setpoint.
- Joystick left:
   Press the button to decrease the temperature setpoint.
- Joystick down:
   Press the button to change the thermostat mode.

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
 


 
	   
