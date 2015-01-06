//"Double-A Datalogger" by Jack Christensen is licensed under
//CC BY-SA 4.0, http://creativecommons.org/licenses/by-sa/4.0/

//util.ino - various utility functions

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

//read battery voltage using ADC6, ADC7 and voltage divider.
//resistors R4 and R5 form the voltage divider.
//NOTE: When switching from the DEFAULT to the INTERNAL 1.1V ADC reference, it can take
//5-10ms for Aref to stabilize because it is held up by a 100nF capacitor on the board.
int readBattery(void)
{
    const int R4 = 47500;    //ohms
    const int R5 = 10000;    //ohms
    long adc6, adc7;
    
    analogReference(INTERNAL);
    adc6 = analogRead(6);
    adc7 = analogRead(7);
    return ((adc7 - adc6) * (R4 + R5) / R5 + adc6) * 1100 / 1024;
}

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

