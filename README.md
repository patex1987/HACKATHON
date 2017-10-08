# HACKATHON

## SMART SENSOR

* Hardware of smart meter
This consists of three parts:
1, Transformers: current transformer 1:50 with 47Ohm load resistor and voltage transformer 230/6V with 220Ohm load resistor. Those are providing galvanic isolation from mains side and suitable voltage levels for ADC.
2, ADC - see schematics in ADC.pdf. Nothing but basic connection around MCP3901 device. Two voltage supply levels are involved - 5V for analog part, 3V for digital IO.
3, SMT32L100 discvery board. Connect to ADC as follows

    PA4 - CS
    PA5 - SCK
    PA6 - MISO
    PA7 - MOSI
    PA3 - DR
    PA2 - NRST
    5V -  5V analog
    3V - 3V VDD
    GND - GND

Connect USB/serial converter as here:
     GND - GND
     PB11 -> RX
     PB10 <-TX

The Discovery kit is to be supplied from USB line. 

* Firmware of smart meter
Import directory firmware/ into STM32 workbench (or any similar Eclipse + GCC alike), build and upload via usual means - ST-link onboard. The firmware expects communication via USART3 (PB10/PB11) at 3V CMOS levles and 115,2kBaud rate.

* Software of smart meter
The C preprocessor/readout is in single C file, named readout.c in readout/ directory. You can compile it by invoking
    gcc readout.c -lm -o readout
from any reasonable Linux distro; or after installing MinGW from windows too. The readout is then called for example
    readout -c /dev/ttyUSB0
from Linux (add user to dialout grout to access serial ports) or
    readout -c COM5 
from Windows.
