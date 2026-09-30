# Must-pv1800-5248-pro

inspired by:
    https://github.com/taHC81/MUST-ESPhome
    https://github.com/vladyspavlov/esphome-must-inverter

pv1800-inverter is a true RS-485 esp32 client device:
To build use TTL-TO-RS485 converter board and esp32 board. 
Connect USB white wire to B-, green wire to A+ and also both USB power (GND and +5V) to 1 and 4 pin TTL-TO-RS485 board.
Also power up your esp32 module (do not mix 3.3V and 5V - many esp32 modules has internal 5V-3.3V converter). Other 2 pins of TTL-TO-RS485 are TxD and RxD should be connected to available pins of your esp32 board.
After uploading firmware to esp32 module at least TXD led on TTL-TO-RS485 board should be blinking. If other is steady and stays dark you should restart your inverter with full hardware stop (reattach your load, then turn off the inverter and finally disconnect the battery - you may turn off BMS for that)

JK BMS is a BLE firmware to read data from the battery using bluetooth BLE connection.

inverter-monitor is 2-in-1 esp32 firmware which combines first one and second one in a single module



