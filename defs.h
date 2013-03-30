/*----------------------------------------------------------------------*
 * defs.h -- system parameters. these should not normally be changed.   *
 *----------------------------------------------------------------------*/

#ifndef defs_h
#define defs_h

#define RTC_TYPE 79412        //set to 79412 for MCP79412 or 3232 for DS3232 only.
#if RTC_TYPE == 79412
#define RTC_RAM_STATUS 0x00   //address in the RTC SRAM to keep log status
#else
#define RTC_RAM_STATUS 0x14
#endif

#define DEBUG_MODE 0
#define BAUD_RATE 57600               //speed for serial interface, must be <= 57600 with 8MHz system clock

//MCU pin assignments
#define RED_LED 6
#define GRN_LED 7
#define SPARE_LED 8
#define DS18B20_DQ 9                  //DS18B20 data pin
#define DS18B20_GND 10                //DS18B20 ground pin
#define BOOST_REGULATOR 11            //high enables the regulator, low passes battery voltage through
#define L_LED LED_BUILTIN
#define PERIP_POWER A1                //RTC and EEPROM power is supplied from this pin
#define START_BUTTON A2
#define DWNLD_BUTTON A3

//state, switch & LED timing
#define STATE_TIMEOUT 30              //POWER_DOWN after this many seconds in COMMAND or SET_TIME mode
#define DEBOUNCE_MS 25                //tact button debounce, milliseconds
#define LONG_PRESS 2000               //ms for a long button press
#define BLIP_ON 100                   //ms to blip LED on
#define BLIP_OFF 900                  //ms to blip LED off
#define ALT_LED 250                   //ms to alternate LEDs
#define LOG_BLINK 500                 //ms to blink LED when record is logged
#define N_LOG_BLINK 5                 //blink the LED for this many records after logging starts

//MCU system clock prescaler values
#define CLOCK_8MHZ 0                  //CLKPS[3:0] value for divide by 1
#define CLOCK_1MHZ 3                  //CLKPS[3:0] value for divide by 8

#endif

