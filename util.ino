//util.ino - various utility functions

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

#if RTC_TYPE == 79412
/*----------------------------------------------------------------------*
 * Dump the first nBytes of RTC SRAM                                    *
 *----------------------------------------------------------------------*/
#define RTC_ADDR 0x6F
void dumpRTC(uint8_t nBytes)
{
    uint8_t d;                   //data byte from the RTC
    
    Serial << endl << "RTC Registers";
    for (uint8_t a=0; a<nBytes; a+=8) {
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

//read battery voltage using ADC6, ADC7 and voltage divider
//resistors R4 and R5 form the voltage divider
#define R4 47500            //ohms
#define R5 10000            //ohms
int readBattery(void)
{
    long adc6, adc7;
    
    analogReference(INTERNAL);
    adc6 = analogRead(6);
    adc7 = analogRead(7);
    return ((adc7 - adc6) * (R4 + R5) / R5 + adc6) * 1100 / 1024;
}

#if DEBUG_MODE == 1
/*----------------------------------------------------------------------*
 * Dump EEPROM starting at addr for nBytes.                             *
 * (Always dumps a multiple of eight bytes.)                            *
 *----------------------------------------------------------------------*/
extEEPROM ep = extEEPROM (32, 1, 64);
void dumpEEPROM(unsigned long addr, unsigned int nBytes)
{
    uint8_t d;                   //data byte from the EEPROM
    unsigned long a;
    
    Serial << endl << "EEPROM Dump" << endl;
    a = addr;
    while (a < addr + nBytes) {
        Serial << "0x";                    //print address
        if (a < 16*16*16) Serial << '0';
        if (a < 16*16) Serial << '0';
        if (a < 16) Serial << '0';
        Serial << _HEX(a);
        
        for (uint8_t i=0; i<8; i++) {
            ep.read(a++, &d, 1);
            Serial << ( d < 0x10 ? " 0" : " ") << _HEX(d);
        }
        Serial << endl;
    }
}
#endif

void createData(int nrec)
{
    int i;
    time_t rtcTime = RTC.get();
    
    for (i=0; i<nrec; i++) {
        LOGDATA.fields.timestamp = rtcTime;
        LOGDATA.fields.tempSensor = i;
        LOGDATA.fields.tempRTC = i;
        LOGDATA.fields.vBat2 = i;
        LOGDATA.fields.vReg = i;
        if (!LOGDATA.write()) {
            Serial << F("write fail in createData") << endl;
            STATE = POWER_DOWN;
            return;
        }
        else if (i % 100 == 0) {
            Serial << F("createData:") << i << endl;
            rtcTime = RTC.get();
        }
    }
    Serial << F("createData:") << i << endl;
}
