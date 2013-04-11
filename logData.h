//logData.h - A class to define the log data structure and
//provide methods for managing it.

#ifndef logData_h
#define logData_h
#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

//select RTC by commenting one of the next two lines, and setting the #define accordingly.
//make similar changes in the _main file also.
//#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <MCP79412RTC.h>      //http://github.com/JChristensen/MCP79412RTC

#include <extEEPROM.h>        //http://github.com/JChristensen/extEEPROM
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Timezone.h>         //http://github.com/JChristensen/Timezone
#include "defs.h"
#include "config.h"

class logData
{
    public:
        logData(unsigned long eepromCapacity, boolean wrapWhenFull);
        void initialize(void);
        boolean write(void);
        void download(Timezone *tz);
        boolean readLogStatus(boolean printStatus);
        boolean configChanged(boolean printStatus);

        union {
            logData_t fields;
            byte bytes[sizeof(logData_t)];
        };
        
    private:
        boolean readFirst(void);
        boolean readNext(void);
        void writeLogStatus(boolean writeConfig);
        void print8601(time_t t);
        void printI00(int val, char delim);
        unsigned long _eepromCapacity;    //EEPROM capacity in bytes (total for all EEPROM devices combined)
        static const byte _logRecSize = sizeof(logData_t);
        boolean _wrapMode;                //true: logging continues when EEPROM is full, next record replacing the oldest
                                          //false: logging stops at the top EEPROM memory address
        unsigned long _nextRecord;        //EEPROM address where the next log data record will be written
        boolean _eepromFull;
        unsigned long _readRecord;        //for reading/downloading
    
        union {                           //logging status data persisted in RTC SRAM (battery-backed)
            struct {
                unsigned long eepromCapacity; //copy of _eepromCapacity
                byte recSize;                 //copy of _logRecSize
                boolean wrap;                 //copy of _wrapWhenFull
                unsigned long next;           //copy of _nextRecord
                boolean full;                 //copy of _eepromFull
            };
            byte bytes[11];
        } logStatus;
};

extern logData LOGDATA;

#endif

