# WeatherFlow-Gauges

Drive a set of analog gauges from the UDP broadcast reports from a WeatherFlow Tempest weather station, using an ESP32 microcontroller.  Current gauges are for Wind Speed (MPH), Wind Direction (cardinal heading), and Temperature (degrees F).  The wind direction assumes there will be 8 LEDs, driven from an SPI output register.

The design is based the [TinyPICO](https://www.tinypico.com/) board.
Analog outputs are created from PWM output, combined with a low pass
filter.  Eight LEDs are driven from through an MCP23008 GPIO chip.  Two
additional LED (or general purpose digital outputs) are also provided
by GPIO pins on the TinyPICO. along with two digial inputs (buttons).

Power connections are J1, providing 5 to 12 volts DC, or through the
USB connection on the TinyPICO.

## PCB Design

The ``pcb/pdf`` directory provides the schematics and design layout of
the board.  The ``pcb/gerbers`` directory contains the production files
to spin the PCB board.

## Firmware

The firmware (this repo) is built via [PlatformIO](https://platformio.org/)
and VS Code.  Programming can be via the USB interfaces over over
the air, by selecting the built enviroment *tinypico_ota*.

## Operation & Setup

Settings are stored persistenly in the ESP32 EEPROM.  If the system
does not have a valid configuration, it will enable AP mode,
broadcasting the open (no encryption or passkey) SSID
*WeatherFlowGauges*.  After connecting to the system, the system
can be configured through a web broswer via
[http://192.168.4.1](http://192.168.4.1).  The initial username and
password is *admin*/*temp*.

System status is provided through the RBG LED on the TinyPICO board.

| LED Color         | Meaning
| ----------------- | --------
| Solid Blue        | Trying to connect to Wi-Fi
| Pulsing Green     | System connected to Wi-Fi, normal operation
| Color Wheel Cycle | Running as Wi-Fi AP, waiting for setup
| Solid Orange      | Factory reset started
| Solid Red         | Factory reset completed

### Factory Reset

While holding down **Button 1**, press the **Reset** button.  The
RBD LED will turn orange, keep holding Button 1 for at least 5
seconds until the RGB LED turns red, then release. The system has
now been reset back to the factory default settings.

## Gauages

TBD
