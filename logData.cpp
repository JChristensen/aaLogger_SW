//logData.cpp - A class to define the log data structure and
//provide methods for managing it.

#include "logData.h"

//instantiate the logData and extEEPROM objects
logData LOGDATA = logData ( EEPROM_SIZE * NBR_EEPROM, WRAP_MODE );
extEEPROM EEEP = extEEPROM ( EEPROM_SIZE, NBR_EEPROM, EEPROM_PAGE );

// the constructor specifies the total EEPROM capacity (all devices
// combined) in kB, and whether wrap mode is enabled (wrap mode
// causes the oldest log record to be overwritten when EEPROM memory
// is full).
//
// Whenever the EEPROM size or wrap mode is changed an INITIALIZE
// is required before logging can begin.
logData::logData(unsigned long eepromCapacity, boolean wrapMode)
{
    _eepromCap = eepromCapacity * 1024UL;
    _maxRec = _eepromCap / _recSize;
    _topAddr = (_maxRec - 1) * _recSize;
    _wrapMode = wrapMode;
}

//reset EEPROM status to empty
void logData::initialize(void)
{
    _firstAddr = 0;
    _lastAddr = 0;
    _nRec = 0;
    writeLogStatus(true);    //include log config parameters
}

//read the first log record.
//returns false if there is no data logged.
boolean logData::readFirst(void)
{
    if (_nRec == 0)
        return false;
    else {
        _readAddr = _firstAddr;
        EEEP.read(_readAddr, LOGDATA.bytes, _recSize);
        return true;
    }
}
        
//read the next log record.
//returns false if there is no more data to be read.
boolean logData::readNext(void)
{
    if (_readAddr == _lastAddr)
        return false;
    else {
        _readAddr += _recSize;
        if (_readAddr > _topAddr) {
            if (_wrapMode)
                _readAddr = 0;
            else
                return false;
        }
        EEEP.read(_readAddr, LOGDATA.bytes, _recSize);
        return true;
    }
}

//write a log record into the next eeprom slot.
//returns zero if successful,
//returns EEPROM_ADDR_ERR if eeprom is full and wrap mode is off,
//returns status from Wire library or extEEPROM library if other errors occur.
byte logData::write(void)
{
    byte stat;
    
    if (_wrapMode) {
        if (_nRec > 0) {                    //writing the first record is a special case, don't need to move pointers
            _lastAddr += _recSize;
            if (_lastAddr > _topAddr) _lastAddr = 0;
        }
        stat = EEEP.write(_lastAddr, LOGDATA.bytes, _recSize);    //write the record
        if (stat != 0)
            return stat;                                          //something wrong, return the error status
        else {
            if (_nRec >= _maxRec) {         //eeprom full?
                _firstAddr += _recSize;     //yes, make room
                if (_firstAddr > _topAddr) _firstAddr = 0;
            }
            else
                ++_nRec;                    //no, keep counting
            writeLogStatus(false);
            return stat;
        }
    }
    else {
        if (_nRec > 0) {                    //first rec is special case
            if (_nRec >= _maxRec)           //have room?
                return EEPROM_FULL_ERR;     //no
            else
                _lastAddr += _recSize;
        }
        stat = EEEP.write(_lastAddr, LOGDATA.bytes, _recSize);    //write the record
        if (stat != 0)
            return stat;                                          //something wrong, return the error status
        else {
            ++_nRec;
            writeLogStatus(false);
            return stat;
        }
    }
}

//send the logged data to the serial monitor in CSV format.
//pass a pointer to a local time zone object to allow timestamps to be output in local time.
//when changing the log data structure, the code block below with
//comment (1) will need modification.
void logData::download(Timezone *tz)
{
    unsigned long ms, msLast;
    unsigned long nRec = 0;
    boolean ledState;
    TimeChangeRule *tcr;                  //pointer to the time change rule, use to get TZ abbrev
    
    if (readFirst()) {
        Serial << F(CSV_HEADER) << endl;

        do {
            ++nRec;

            { /*---- (1) PRINT CSV DATA TO SERIAL MONITOR ----*/
                print8601(LOGDATA.fields.timestamp);
                print8601((*tz).toLocal(LOGDATA.fields.timestamp, &tcr));
                Serial << tcr -> abbrev << ',';
                Serial << _DEC(LOGDATA.fields.tempRTC) << ',';
                Serial << _DEC(LOGDATA.fields.tempDS) << ',';
                Serial << _DEC(LOGDATA.fields.ldr) << ',';
                Serial << _DEC(LOGDATA.fields.vBat) << ',';
                Serial << _DEC(LOGDATA.fields.vReg) << endl;
            }

            ms = millis();                //flash LEDs while downloading data
            if (ms - msLast > BLIP_ON) {
                msLast = ms;
                digitalWrite(RED_LED, ledState = !ledState);
                digitalWrite(GRN_LED, !ledState);
            }
        } while (readNext());
        digitalWrite(RED_LED, LOW);
        digitalWrite(GRN_LED, LOW);
        Serial << F("*EOF ") << _DEC(nRec) << F(" Record") << (_nRec==1 ? "" : "s") << '.' << endl;
    }
    else {
        Serial << endl << F("NO DATA LOGGED") << endl;
    }
}

//write the log status (pointer to next record, etc.) to the RTC's SRAM.
//optionally include the EEPROM and log config parameters (used for initialization only)
void logData::writeLogStatus(boolean writeConfig)
{
    if (writeConfig) {                         //include configuration parameters
        logStatus.eepromCap = _eepromCap;
        logStatus.maxRec = _maxRec;
        logStatus.topAddr = _topAddr;
        logStatus.recSize = _recSize;
        logStatus.wrapMode = _wrapMode;
    }
    logStatus.firstAddr = _firstAddr;          //current status
    logStatus.lastAddr = _lastAddr;
    logStatus.nRec = _nRec;
    RTC.writeRTC(RTC_RAM_STATUS, logStatus.bytes, sizeof(logStatus));
}

//read and optionally print the log status (pointer to next record, etc.) from the RTC's SRAM.
//if not in wrap mode and eeprom is full, returns false, else true.
boolean logData::readLogStatus(boolean printStatus)
{
    unsigned long pctAvail;
    
    RTC.readRTC(RTC_RAM_STATUS, logStatus.bytes, sizeof(logStatus));
    _firstAddr = logStatus.firstAddr;
    _lastAddr = logStatus.lastAddr;
    _nRec = logStatus.nRec;
    if (printStatus) {
        pctAvail = _nRec * 10000UL / _maxRec;
        pctAvail = (10000UL - pctAvail + 5 ) / 10;
        Serial << endl << _DEC(_eepromCap >> 10) << F("kB EEPROM, ");
        Serial << _DEC(_nRec) << F(" Record") << (_nRec==1 ? "" : "s") << F(" logged, ");
        Serial << _DEC(pctAvail / 10) << '.' << (pctAvail % 10) << F("% Available.") << endl;
        Serial << F("Log interval ");
        printI00(hour(LOG_INTERVAL), ':');
        printI00(minute(LOG_INTERVAL), ':');
        printI00(second(LOG_INTERVAL), ',');
        Serial << F(" Record size ") << _DEC(_recSize) << F(" bytes, ");
        if (!_wrapMode) Serial << F("NO-");
        Serial << F("WRAP mode.") << endl;
    }
    if (!_wrapMode && (_nRec >= _maxRec))
        return false;
    else
        return true;
}

//check the current configuration against that read from RTC SRAM, return true if different.
boolean logData::configChanged(boolean printStatus)
{
    readLogStatus(printStatus);
    if ( logStatus.eepromCap != _eepromCap || logStatus.recSize != _recSize || logStatus.wrapMode != _wrapMode) {
        Serial << F("Configuration changed, INITIALIZE required.") << endl;
        return true;
    }
    else
        return false;
}

//print a timestamp to serial in ISO8601 format, followed by a comma
void logData::print8601(time_t t)
{
    printI00(year(t), '-');
    printI00(month(t), '-');
    printI00(day(t), ' ');
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ',');
}

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void logData::printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}

