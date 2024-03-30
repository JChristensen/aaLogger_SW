// Double-A DataLogger: A low-power Arduino-based data logger.
// https://github.com/JChristensen/aaLogger_SW
// Copyright (C) 2013-2024 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

// logData.h - A class to define the log data structure and
// provide methods for managing it.

#ifndef logData_h
#define logData_h
#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <DS3232RTC.h>      // http://github.com/JChristensen/DS3232RTC
#include <extEEPROM.h>      // http://github.com/JChristensen/extEEPROM
#include <Streaming.h>      // https://github.com/janelia-arduino/Streaming
#include <TimeLib.h>        // http://playground.arduino.cc/Code/Time
#include <Timezone.h>       // http://github.com/JChristensen/Timezone
#include "defs.h"
#include "config.h"

// EEPROM full error, returned by write() if not in wrap mode and EEPROM is full
#define EEPROM_FULL_ERR 8

class logData
{
    public:
        logData(unsigned long eepromCapacity, bool wrapMode);
        void initialize();
        byte write();
        void download(Timezone *tz);
        bool readLogStatus(bool printStatus);
        bool configChanged(bool printStatus);

        union {
            logData_t fields;
            byte bytes[sizeof(logData_t)];
        };

    private:
        bool readFirst();
        bool readNext();
        void writeLogStatus(bool writeConfig);
        void print8601(time_t t);
        void printI00(int val, char delim);

        unsigned long _firstAddr;       // pointer to the oldest record in EEPROM
        unsigned long _lastAddr;        // pointer to the newest record in EEPROM
        unsigned long _nRec;            // number of records stored in EEPROM
        unsigned long _eepromCap;       // EEPROM capacity in bytes (total for all EEPROM devices combined)
        unsigned long _maxRec;          // maximum number of records that will fit in EEPROM
        unsigned long _topAddr;         // pointer to the last EEPROM location that can hold a whole record
        static const byte _recSize = sizeof(logData_t); // size of log record in bytes
        bool _wrapMode;                 // true: continue logging when EEPROM is full, next record replacing the oldest
                                        // false: logging stops when EEPROM is full
        unsigned long _readAddr;        // pointer to read records

        union {                         // logging status data persisted in RTC SRAM (battery-backed)
            struct {
                unsigned long firstAddr;    // copies of variables above
                unsigned long lastAddr;
                unsigned long nRec;
                unsigned long eepromCap;
                unsigned long maxRec;
                unsigned long topAddr;
                byte recSize;
                bool wrapMode;
            };
            byte bytes[26];
        } logStatus;
};

extern logData LOGDATA;
extern extEEPROM EEEP;
extern DS3232RTC myRTC;

#endif
