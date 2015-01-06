//"Double-A Datalogger" by Jack Christensen is licensed under
//CC BY-SA 4.0, http://creativecommons.org/licenses/by-sa/4.0/

/*----------------------------------------------------------------------*
 * defs.h -- system parameters. these should not normally be changed.   *
 *----------------------------------------------------------------------*/

#ifndef defs_h
#define defs_h

//MCU pin assignments
#define PERIP_POWER 2                 //RTC and EEPROM power is supplied from this pin
#define BOOST_REGULATOR 4             //high enables the regulator, low passes battery voltage through
#define DWNLD_BUTTON 5
#define START_BUTTON 6
#define RED_LED 7
#define GRN_LED 8
#define SENSOR_POWER 9                //drives the mosfet to apply power to the sensors
#define DS18B20_DQ 11                 //DS18B20 data pin

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

//other
#define DEBUG_MODE 0
#define SOFTWARE_VERSION 1
#define BAUD_RATE 57600               //speed for serial interface, must be <= 57600 with 8MHz system clock
#define RTC_RAM_STATUS 0x14           //address in the RTC SRAM to keep log status

#endif

