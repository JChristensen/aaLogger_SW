//logData.cpp - A class to define the log data structure and
//provide methods for managing it.

#include "logData.h"
#include <MCP79412RTC.h>      //http://github.com/JChristensen/MCP79412RTC
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/

/*----------------------------------------------------------------------*
 * Instantiate the logData object below. The constructor specifies the  *
 * total EEPROM size (all devices combined) in kB, and whether wrap     *
 * mode is enabled (wrap mode causes the oldest log record to be        *
 * overwritten when EEPROM memory is full).                             *
 *                                                                      *
 * IMPORTANT! Whenever the EEPROM size or wrap mode is changed,         *
 * do not start logging until an INITIALIZE has been done.              *
 *----------------------------------------------------------------------*/
logData LOGDATA = logData(32 * 1024UL, false);
//logData LOGDATA = logData(64UL, false);

#define RED_LED 6            //duplicated from main module
#define GRN_LED 7
#define BLIP_ON 100

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
    writeLogStatus();
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
            writeLogStatus();
            return false;                //yes, tell the caller
        }
    }
    writeLogStatus();
    return true;
}

//send the logged data to the serial monitor in CSV format.
//pass a pointer to a local time zone object to allow timestamps to be output in local time.
void logData::download(Timezone *tz)
{
    unsigned long ms, msLast;
    boolean ledState;
    TimeChangeRule *tcr;                  //pointer to the time change rule, use to get TZ abbrev
    
    if (readFirst()) {
        Serial << F("utc,local,tz,sensorTemp,rtcTemp,batteryVoltage,regulatorVoltage") << endl;
        do {
            print8601(LOGDATA.fields.timestamp);
            print8601((*tz).toLocal(LOGDATA.fields.timestamp, &tcr));
            Serial << tcr -> abbrev << ',';
            Serial << _DEC(LOGDATA.fields.sensorTemp) << ',';
            Serial << _DEC(LOGDATA.fields.rtcTemp) << ',';
            Serial << _DEC(LOGDATA.fields.batteryVoltage) << ',';
            Serial << _DEC(LOGDATA.fields.regulatorVoltage) << endl;
            //provide the user with a small light show while downloading data
            ms = millis();
            if (ms - msLast > BLIP_ON) {
                msLast = ms;
                digitalWrite(RED_LED, ledState = !ledState);
                digitalWrite(GRN_LED, !ledState);
            }
        } while (readNext());
        digitalWrite(RED_LED, LOW);
        digitalWrite(GRN_LED, LOW);
    }
    else {
        Serial << F("No data has been logged.") << endl;
    }
    
}

//write the log status (pointer to next record, etc.) to the RTC's SRAM
void logData::writeLogStatus(void)
{
    #if DEBUG_MODE == 1
    Serial << F("Write: next rec ") << _nextRecord << F(" full ") << _eepromFull << endl;
    #endif
    logStatus.next = _nextRecord;
    logStatus.full = _eepromFull;
    RTC.sramWrite(0, logStatus.bytes, sizeof(logStatus));
}

//read and optionally print the log status (pointer to next record, etc.) from the RTC's SRAM.
//if not in wrap mode and eeprom is full, returns false, else true.
boolean logData::readLogStatus(boolean printStatus)
{
    RTC.sramRead(0, logStatus.bytes, sizeof(logStatus));
    _nextRecord = logStatus.next;
    _eepromFull = logStatus.full;
    #if DEBUG_MODE == 1
    Serial << F("Read: next rec ") << _nextRecord << F(" full ") << _eepromFull << endl;
    #endif
    if (printStatus) {
        Serial << F("Records in EEPROM: ");
        if (_eepromFull)
            Serial << _DEC(_eepromSize/_logRecSize) << F(" (FULL)") << endl;
        else
            Serial << _DEC(_nextRecord/_logRecSize) << endl;
    }
    if (_eepromFull && !_wrapWhenFull)
        return false;
    else
        return true;
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


