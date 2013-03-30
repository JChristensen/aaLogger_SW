//logData.cpp - A class to define the log data structure and
//provide methods for managing it.

#include "logData.h"

//instantiate the logData and extEEPROM objects
logData LOGDATA = logData ( EEPROM_SIZE * 1024UL, WRAP_MODE );
extEEPROM EEEP = extEEPROM(EEPROM_SIZE, EEPROM_PAGE);

// the constructor specifies the total EEPROM size (all devices
// combined) in kB, and whether wrap mode is enabled (wrap mode
//causes the oldest log record to be overwritten when EEPROM memory
// is full).
//
// Whenever the EEPROM size or wrap mode is changed an INITIALIZE
// is required before logging can begin.
logData::logData(unsigned long eepromSize, boolean wrapWhenFull)
{
    _eepromSize = eepromSize;
    _wrapWhenFull = wrapWhenFull;
}

//reset EEPROM status to empty
void logData::initialize(void)
{
    _nextRecord = 0;
    _eepromFull = false;
    writeLogStatus(true);    //include log config parameters
}

//read the first log record.
//returns false if there is no data logged.
boolean logData::readFirst(void)
{
    if ( (_nextRecord == 0) && !_eepromFull)
        return false;
    else if (_wrapWhenFull)
        _readRecord = _nextRecord;
    else
        _readRecord = 0;
    
    #if DEBUG_MODE == 1
    Serial << _readRecord << ' ';
    #endif
    EEEP.read(_readRecord, LOGDATA.bytes, _logRecSize);
    return true;
}

//read the next log record.
//returns false if there is no more data to be read.
boolean logData::readNext(void)
{
    _readRecord += _logRecSize;
    if (_readRecord >= _eepromSize) _readRecord = 0;
        
    if (_readRecord == _nextRecord)
        return false;
    else {
        #if DEBUG_MODE == 1
        Serial << _readRecord << ' ';
        #endif
        EEEP.read(_readRecord, LOGDATA.bytes, _logRecSize);
        return true;
    }    
}

//write a log record into the next eeprom slot.
//returns false if eeprom is full and wrap mode is off.
boolean logData::write(void)
{
    if (!_wrapWhenFull && _eepromFull) return false;    //no room at the inn
    
    EEEP.write(_nextRecord, LOGDATA.bytes, _logRecSize);
    _nextRecord += _logRecSize;

    if (_nextRecord >= _eepromSize) {    //past the top of eeprom?
        _eepromFull = true;              //yes, next record goes to addr 0
        _nextRecord = 0;
        if (!_wrapWhenFull) {            //but is wrap mode off?
            writeLogStatus(false);       //save just the current pointer and eeprom-full status
            return false;                //yes, tell the caller
        }
    }
    writeLogStatus(false);
    return true;
}

//send the logged data to the serial monitor in CSV format.
//pass a pointer to a local time zone object to allow timestamps to be output in local time.
//when changing the log data structure, the code blocks below with
//comments (1) and (2) will need modification.
void logData::download(Timezone *tz)
{
    unsigned long ms, msLast;
    unsigned long nRec = 0;
    boolean ledState;
    TimeChangeRule *tcr;                  //pointer to the time change rule, use to get TZ abbrev
    
    if (readFirst()) {

        { /*---- (1) CSV HEADER ----*/
            Serial << F("utc,local,tz,sensorTemp,rtcTemp,batteryVoltage,regulatorVoltage") << endl;
        }

        do {
            ++nRec;

            { /*---- (2) CSV DATA ----*/
                print8601(LOGDATA.fields.timestamp);
                print8601((*tz).toLocal(LOGDATA.fields.timestamp, &tcr));
                Serial << tcr -> abbrev << ',';
                Serial << _DEC(LOGDATA.fields.sensorTemp) << ',';
                Serial << _DEC(LOGDATA.fields.rtcTemp) << ',';
                Serial << _DEC(LOGDATA.fields.batteryVoltage) << ',';
                Serial << _DEC(LOGDATA.fields.regulatorVoltage) << endl;
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
        Serial << F("*EOF ") << _DEC(nRec) << F(" Records.") << endl;
    }
    else {
        Serial << F("NO DATA LOGGED") << endl;
    }
}

//write the log status (pointer to next record, etc.) to the RTC's SRAM.
//optionally include the EEPROM and log config parameters (for initialization only)
void logData::writeLogStatus(boolean writeConfig)
{
    #if DEBUG_MODE == 1
    Serial << F("Write: next rec ") << _nextRecord << F(" full ") << _eepromFull << endl;
    #endif
    if (writeConfig) {                         //include configuration parameters
        logStatus.eepromSize = _eepromSize;
        logStatus.recSize = _logRecSize;
        logStatus.wrap = _wrapWhenFull;
    }
    logStatus.next = _nextRecord;              //current status
    logStatus.full = _eepromFull;
    #if RTC_TYPE == 79412
    RTC.sramWrite(RTC_RAM_STATUS, logStatus.bytes, sizeof(logStatus));
    #else
    RTC.writeRTC(RTC_RAM_STATUS, logStatus.bytes, sizeof(logStatus));
    #endif
}

//read and optionally print the log status (pointer to next record, etc.) from the RTC's SRAM.
//if not in wrap mode and eeprom is full, returns false, else true.
boolean logData::readLogStatus(boolean printStatus)
{
    unsigned long recordsLogged;
    unsigned long pctAvail;
    
    #if RTC_TYPE == 79412
    RTC.sramRead(RTC_RAM_STATUS, logStatus.bytes, sizeof(logStatus));
    #else
    RTC.readRTC(RTC_RAM_STATUS, logStatus.bytes, sizeof(logStatus));
    #endif
    _nextRecord = logStatus.next;
    _eepromFull = logStatus.full;
    #if DEBUG_MODE == 1
    Serial << F("Read: next rec ") << _nextRecord << F(" full ") << _eepromFull << endl;
    #endif
    if (printStatus) {
        recordsLogged = _eepromFull ? _eepromSize / _logRecSize : _nextRecord/_logRecSize;
        pctAvail = (recordsLogged * 10000UL) / (_eepromSize / _logRecSize);
        pctAvail = (10000UL - pctAvail + 5 ) / 10;
        Serial << _DEC(_eepromSize >> 10) << F("kB EEPROM, ");
        Serial << _DEC(pctAvail / 10) << '.' << (pctAvail % 10) << F("% available.") << endl;
        Serial << _DEC(recordsLogged) << F(" Record") << (recordsLogged==1 ? "" : "s");
        Serial << F(" logged, Record size ") << _DEC(_logRecSize) << F(" bytes, ");
        if (!_wrapWhenFull) Serial << F("NO-");
        Serial << F("WRAP mode.") << endl;
    }
    if (_eepromFull && !_wrapWhenFull)
        return false;
    else
        return true;
}

//check the current configuration against that read from RTC SRAM, return true if different.
boolean logData::configChanged(boolean printStatus)
{
    readLogStatus(printStatus);
    if ( logStatus.eepromSize != _eepromSize || logStatus.recSize != _logRecSize || logStatus.wrap != _wrapWhenFull) {
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

