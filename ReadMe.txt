/*----------------------------------------------------------------------*
 * Double-A Datalogger - A low-power Arduino-based data logger.         *
 * Jack Christensen 20Mar2013 v1                                        *
 *                                                                      *
 * This basic logging sketch logs the date/time, the temperature from   *
 * the DS3232 sensor, the battery and regulator voltages.               *
 *                                                                      *
 * Double-A DataLogger Features:                                        *
 * - Runs on two AA alkaline cells.                                     *
 * - DS3232 real-time clock provides timing for logging and battery-    *
 *   backed SRAM to persist EEPROM pointers and status through MCU      *
 *   resets and main battery changes.                                   *
 * - Data is logged to external EEPROM (1 or 2 x M24M02 for up to       *
 *   512kB total).                                                      *
 * - MCP1640C boost regulator provides 3.3V or 5V, but has a bypass     *
 *   mode which puts the regulator into a low-power shutdown mode and   *
 *   passes the battery voltage straight through to save power while    *
 *   the MCU sleeps between data samples.                               *
 *                                                                      *
 * Developed with Arduino 1.0.3.                                        *
 * Set ATmega328P Fuses (L/H/E): 0x7F, 0xDE, 0x06.                      *
 * Uses an 8MHz crystal with CKDIV8 bit programmed, so the system clock *
 * is 1MHz after reset. This is to ensure the MCU is not overclocked    *
 * at low voltages when the boost regulator is disabled. Clock is       *
 * changed to 8MHz when the regulator is enabled. Because of this,      *
 * the board MUST be programmed via ICSP (no bootloader), and the code  *
 * MUST be compiled for an 8MHz ATmega328P (the MCU runs at 8MHz most   *
 * of the time, only switching to 1MHz just before sleeping).           *
 *                                                                      *
 * CC BY-SA 4.0                                                         *
 * "Double-A Datalogger" by Jack Christensen is licensed under          *
 * CC BY-SA 4.0, http://creativecommons.org/licenses/by-sa/4.0/         *
 *----------------------------------------------------------------------*/