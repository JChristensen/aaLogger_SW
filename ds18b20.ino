// Double-A DataLogger: A low-power Arduino-based data logger.
// https://github.com/JChristensen/aaLogger_SW
// Copyright (C) 2013-2026 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

// ds18b20.ino - Functions for a DS18B20 temperature sensor.
// To save a bit more power, uses the watchdog timer to sleep the
// MCU for a second while the DS18B20 does the temperature conversion.

bool readDS18B20(int16_t *tF10)
{
    OneWire ds(DS18B20_DQ);
    uint8_t dsData[12];

    // start temperature conversion
    ds.reset();
    ds.skip();
    ds.write(0x44);

    // sleep while conversion in progress, leave the regulator on for the sensor
    wdtEnable();
    gotoSleep(true);
    wdtDisable();

    // read the results
    ds.reset();
    ds.skip();
    ds.write(0xBE); // read scratchpad

    for (int16_t i=0; i<9; i++) {  // read 9 bytes
        dsData[i] = ds.read();
    }
    if (OneWire::crc8(dsData, 8) == dsData[8]) {
        *tF10 = toFahrenheit(dsData[1], dsData[0]);
        return true;
    }
    else {
        return false;   // CRC error
    }
}

// Convert 12-bit °C temp from DS18B20 to an integer which is °F * 10
int16_t toFahrenheit(uint8_t tempMSB, uint8_t tempLSB)
{
    // 16 times the temperature in deg C (DS18B20 resolution is 1/16 °C)
    int32_t tC16 = (tempMSB << 8) + tempLSB;
    // 160 times the temp in deg F (but without the 32 deg offset)
    int32_t tF160 = tC16 * 18;
    // 10 times the temp in deg F
    int16_t tF10 = tF160 / 16;
    if (tF160 % 16 >= 8) tF10++;    // round up to the next tenth if needed
    tF10 = tF10 + 320;              // add in the offset*10
    return tF10;
}

// enable the wdt for 1 sec interrupt
void wdtEnable()
{
    cli();
    wdt_reset();
    MCUSR = 0x00;
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1); // 128K cycles = 1 sec
    sei();
}

// disable the wdt
void wdtDisable()
{
    cli();
    wdt_reset();
    MCUSR = 0x00;
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR = 0x00;
    sei();
}

// nothing to do here, the WDT interrupt just wakes the MCU after the DS18B20 has completed conversion.
ISR(WDT_vect) {}
