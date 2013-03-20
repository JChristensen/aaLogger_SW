/*----------------------------------------------------------------------*
 * Double-A DataLogger - A low-power Arduino-based data logger.         *
 * Jack Christensen 20Mar2013 v1                                        *
 *                                                                      *
 * Features:                                                            *
 * - Runs on two AA alkaline cells.                                     *
 * - A DS3232 real-time-clock provides timing for logging and battery-  *
 *   backed SRAM to persist EEPROM pointers and status through MCU      *
 *   resets and main battery changes.                                   *
 * - Data is logged to external EEPROM (2 x M24M02 for 512kB total).    *
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
 * changed to 8MHz when the regulator is enabled.                       *
 *                                                                      *
 * This work is licensed under the Creative Commons Attribution-        *
 * ShareAlike 3.0 Unported License. To view a copy of this license,     *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a       *
 * letter to Creative Commons, 171 Second Street, Suite 300,            *
 * San Francisco, California, 94105, USA.                               *
 *----------------------------------------------------------------------*/
