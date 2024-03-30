# Double-A DataLogger
### A low-power Arduino-based data logger.
https://github.com/JChristensen/aaLogger_SW  
README file  

## License
Double-A DataLogger Copyright (C) 2013-2024 Jack Christensen GNU GPL v3.0

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License v3.0 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/gpl.html>

## Features
- Runs on two AA alkaline cells.
- DS3232 real-time clock provides timing for logging and battery-backed SRAM to persist EEPROM pointers and status through MCU resets and main battery changes.
- Data is logged to external EEPROM (1 or 2 x M24M02 for up to 512kB total).
- MCP1640C boost regulator provides 3.3V or 5V, but has a bypass mode which puts the regulator into a low-power shutdown mode and passes the battery voltage straight through to save power while the MCU sleeps between data samples.
- Developed with Arduino 1.0.3; updated for Arduino 1.8.19.

## Compiling
The circuit uses an 8MHz crystal with CKDIV8 bit programmed, so the system clock runs at 1MHz after reset. This is to ensure the MCU is not overclocked at low supply voltages when the boost regulator is disabled. Clock is changed to 8MHz when the regulator is enabled. Because of this, the board must be programmed via ICSP (no bootloader), and the code must be compiled for an 8MHz ATmega328P (the MCU runs at 8MHz most of the time, only switching to 1MHz just before sleeping).

Set ATmega328P Fuses (L/H/E): 0x7F, 0xDE, 0x06:
```
$ avrdude -p m328p -U lfuse:w:0x7f:m -U hfuse:w:0xde:m -U efuse:w:0x06:m -v
```

## See also
[Hardware design](https://github.com/JChristensen/aaLogger_HW).
