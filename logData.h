// Double-A DataLogger: A low-power Arduino-based data logger.
// https://github.com/JChristensen/aaLogger_SW
// Copyright (C) 2013-2026 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

// logData.h - A class to define the log data structure and
// provide methods for managing it.

#ifndef AALOGGER_LOGDATA_H_INCLUDED
#define AALOGGER_LOGDATA_H_INCLUDED

#include <DS3232RTC.h>      // http://github.com/JChristensen/DS3232RTC
#include <JC_EEPROM.h>      // http://github.com/JChristensen/JC_EEPROM
#include <Streaming.h>      // https://github.com/janelia-arduino/Streaming
#include <TimeLib.h>        // http://playground.arduino.cc/Code/Time
#include <Timezone.h>       // http://github.com/JChristensen/Timezone
#include "defs.h"
#include "config.h"

// EEPROM full error, returned by write() if not in wrap mode and EEPROM is full
constexpr uint8_t EEPROM_FULL_ERR {8};

class logData
{
    public:
        logData(uint32_t eepromCapacity, bool wrapMode);
        void initialize();
        uint8_t write();
        void download(Timezone *tz);
        bool readLogStatus(bool printStatus);
        bool configChanged(bool printStatus);
        time_t getLogInterval() {return _logInterval;}
        void putLogInterval(time_t interval);

        union {
            logData_t fields;
            uint8_t bytes[sizeof(logData_t)];
        };

    private:
        bool readFirst();
        bool readNext();
        void writeLogStatus(bool writeConfig);
        void print8601(time_t t);
        void printI00(int16_t val, char delim);

        uint32_t _firstAddr;        // pointer to the oldest record in EEPROM
        uint32_t _lastAddr;         // pointer to the newest record in EEPROM
        uint32_t _nRec;             // number of records stored in EEPROM
        uint32_t _eepromCap;        // EEPROM capacity in bytes (total for all EEPROM devices combined)
        uint32_t _maxRec;           // maximum number of records that will fit in EEPROM
        uint32_t _topAddr;          // pointer to the last EEPROM location that can hold a whole record
        time_t _logInterval;        // logging interval in seconds
        static const uint8_t _recSize{sizeof(logData_t)};  // size of log record in bytes
        bool _wrapMode;             // true: continue logging when EEPROM is full, next record replacing the oldest
                                    // false: logging stops when EEPROM is full
        uint32_t _readAddr;         // pointer to read records
        static constexpr time_t DEFAULT_INTERVAL {60}; // default logging interval in seconds, must be > 0

        union {                     // logging status data persisted in RTC SRAM (battery-backed)
            struct {
                uint32_t firstAddr; // copies of variables above
                uint32_t lastAddr;
                uint32_t nRec;
                uint32_t eepromCap;
                uint32_t maxRec;
                uint32_t topAddr;
                uint8_t recSize;
                bool wrapMode;
                time_t logInterval;
            };
            uint8_t bytes[30];
        } logStatus;
        uint32_t _signature; // used to detect invalid logging status data
        const uint8_t RTC_RAM_STATUS {0x14};        // RTC SRAM address for log status
        const uint8_t RTC_INIT_SIGNATURE {0x40};    // RTC SRAM address for initialization signature
        const uint32_t INITIALIZED {0xaa55aa55};    // value to indicate log data initialized
};

extern logData LOGDATA;
extern JC_EEPROM EEEP;
extern DS3232RTC myRTC;

#endif
