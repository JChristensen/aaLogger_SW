//util - various utility functions

//print date and time to Serial
void printDateTime(time_t t, char *tz)
{
    printDate(t);
    Serial << ' ';
    printTime(t);
    //Serial << tcr -> abbrev << ' ';
    Serial << tz << endl;
}

//print time to Serial
void printTime(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}

//print date to Serial
void printDate(time_t t)
{
    //Serial << dayShortStr(weekday(t)) << ' ';
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}

#if RTC_TYPE == MCP79412
/*----------------------------------------------------------------------*
 * Dump the first 32 bytes of RTC SRAM                                  *
 *----------------------------------------------------------------------*/
#define RTC_ADDR 0x6F
void dumpRTC()
{
    #define N_BYTES 32           //number of bytes to dump
    uint8_t d;                   //data byte from the RTC
    
    Serial << endl << "RTC Registers";
    for (uint8_t a=0; a<N_BYTES; a+=8) {
        Serial << endl << "0x" << (a < 0x10 ? "0" : "" ) << _HEX(a);
        Wire.beginTransmission(RTC_ADDR);
        Wire.write(a);
        Wire.endTransmission();
        Wire.requestFrom(RTC_ADDR, 8);
        for (uint8_t i=0; i<8; i++) {
            d = Wire.read();
            Serial << ( d < 0x10 ? " 0" : " ") << _HEX(d);
        }
    }
    Serial << endl;
}
#else
void dumpRtcRegisters()
{
    uint8_t regs[19];

    Serial << F("RTC Registers");
    RTC.readRTC(0, regs, sizeof(regs));
    for (uint8_t n=0; n<sizeof(regs); n++) {
        if (n % 8 == 0)
            Serial << endl << "0x" << (n<16 ? "0" : "") << _HEX(n);
        Serial << ' ' << (regs[n]<16 ? "0" : "") << _HEX(regs[n]);
    }
    Serial << endl;
}
#endif

//read 1.1V reference against AVcc
//from http://code.google.com/p/tinkerit/wiki/SecretVoltmeter
int readVcc(void)
{
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2);                                 //Vref settling time
    ADCSRA |= _BV(ADSC);                      //start conversion
    loop_until_bit_is_clear(ADCSRA, ADSC);    //wait for it to complete
    return 1126400L / ADC;                    //calculate AVcc in mV (1.1 * 1000 * 1024)
}

