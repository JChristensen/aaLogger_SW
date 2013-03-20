//Functions for the DS18B20 temperature sensor.
//To save a bit more power, uses the watchdog timer to sleep the
//MCU for a second while the DS18B20 does the temperature conversion.

boolean readDS18B20(int *tF10)
{
    OneWire ds(DS18B20_DQ);
    uint8_t dsData[12];
    
    //start temperature conversion
    digitalWrite(DS18B20_GND, LOW);    //power the DS18B20 on
    ds.reset();
    ds.skip();
    ds.write(0x44);

    //sleep while conversion in progress, leave the regulator on for the sensor
    wdtEnable();
    gotoSleep(true);
    digitalWrite(PERIP_POWER, HIGH);    //sleeo shuts off the peripherals
    wdtDisable();

    //read the results
    ds.reset();
    ds.skip();
    ds.write(0xBE); //read scratchpad

    for ( int i=0; i<9; i++) { //read 9 bytes
        dsData[i] = ds.read();
    }
    digitalWrite(DS18B20_GND, HIGH);    //power the DS18B20 off
    if (OneWire::crc8(dsData, 8) == dsData[8]) {
        *tF10 = toFahrenheit(dsData[1], dsData[0]);
        return true;
    }
    else {
        return false; //CRC error
    }
    
}

// Convert 12-bit °C temp from DS18B20 to an integer which is °F * 10
int toFahrenheit(byte tempMSB, byte tempLSB)
{
    int tC16;     //16 times the temperature in deg C (DS18B20 resolution is 1/16 °C)
    int tF160;    //160 times the temp in deg F (but without the 32 deg offset)
    int tF10;     //10 times the temp in deg F

    tC16 = (tempMSB << 8) + tempLSB;
    tF160 = tC16 * 18;
    tF10 = tF160 / 16;
    if (tF160 % 16 >= 8) tF10++; //round up to the next tenth if needed
    tF10 = tF10 + 320; //add in the offset (*10)
    return tF10;
}

//enable the wdt for 1 sec interrupt
void wdtEnable(void)
{
    cli();
    wdt_reset();
    MCUSR = 0x00;
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1);    //128K cycles = 1 sec
    sei();
}

//disable the wdt
void wdtDisable(void)
{
    cli();
    wdt_reset();
    MCUSR = 0x00;
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR = 0x00;
    sei();
}

//nothing to do here, the WDT interrupt just wakes the MCU after the DS18B20 has completed conversion.
ISR(WDT_vect)
{
}

