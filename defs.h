// Double-A DataLogger: A low-power Arduino-based data logger.
// https://github.com/JChristensen/aaLogger_SW
// Copyright (C) 2013-2024 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

// defs.h -- system parameters. these should not normally be changed,
// except for sensor pin assignments.

#ifndef AALOGGER_DEFS_H_INCLUDED
#define AALOGGER_DEFS_H_INCLUDED

// MCU pin assignments (do not change)
constexpr uint8_t
    PERIP_POWER     {2},    // RTC and EEPROM power is supplied from this pin
    BOOST_REGULATOR {4},    // high enables the regulator, low passes battery voltage through
    DWNLD_BUTTON    {5},
    START_BUTTON    {6},
    RED_LED         {7},
    GRN_LED         {8},
    SENSOR_POWER    {9};    // drives the mosfet to apply power to the sensors

// sensor pin assignments
constexpr uint8_t
    DS18B20_DQ      {11},   // DS18B20 data pin
    LDR             {A0};   // CdS LDR on analog pin 0

// state, switch & LED timing
constexpr uint32_t
    STATE_TIMEOUT {30},     // POWER_DOWN after this many seconds in COMMAND or SET_TIME mode
    LONG_PRESS {2000},      // ms for a long button press
    BLIP_ON {100},          // ms to blip LED on
    BLIP_OFF {900},         // ms to blip LED off
    LOG_BLINK {500};        // ms to blink LED when record is logged
constexpr uint8_t N_LOG_BLINK {5};  // blink the LED for this many records after logging starts

// MCU system clock prescaler values
constexpr uint8_t CLOCK_8MHZ {0};   // CLKPS[3:0] value for divide by 1
constexpr uint8_t CLOCK_1MHZ {3};   // CLKPS[3:0] value for divide by 8

// other
constexpr int32_t BAUD_RATE {57600};        // speed for serial interface, must be <= 57600 with 8MHz system clock
constexpr uint8_t RTC_RAM_STATUS {0x14};    // address in the RTC SRAM to keep log status

#endif
