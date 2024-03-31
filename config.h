// Double-A DataLogger: A low-power Arduino-based data logger.
// https://github.com/JChristensen/aaLogger_SW
// Copyright (C) 2013-2024 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

// config.h -- user-definable parameters
// define logging interval, eeprom characteristics and the log data
// structure here.

#ifndef AALOGGER_CONFIG_H_INCLUDED
#define AALOGGER_CONFIG_H_INCLUDED

constexpr time_t LOG_INTERVAL {60}; // logging interval in seconds, must be > 0
constexpr bool WRAP_MODE {false};   // true to overwrite oldest data once EEPROM is full,
                                    // false to stop logging when EEPROM full

// size of one EEPROM in kilobits
constexpr JC_EEPROM::eeprom_size_t EEPROM_KBITS {JC_EEPROM::kbits_2048};
constexpr uint8_t NBR_EEPROM {2};       // NUMBER of EEPROM devices on the I2C bus
constexpr uint16_t EEPROM_PAGE {256};   // EEPROM page size in BYTES

// The struct below defines the log data. When modifying the struct,
// also change the logData::download() function in logData.cpp and the
// logSensorData() function in the main module accordingly.
//
// When using M24M02 EEPROMs, the size of the struct should be a
// multiple of four bytes if at all possible. This will minimize the
// number of write cycles and therefore maximize EEPROM endurance.
// Pad the struct out with a byte array, e.g. uint8_t RFU[2]; if needed.

struct logData_t {
    uint32_t timestamp;
    int16_t tempRTC;
    int16_t vBat;
    int16_t vReg;
    int16_t RFU;    // make the log record a multiple of 4 bytes
};

// this line defines the field names and is printed at the beginning of the data when downloading
#define CSV_HEADER "utc,local,tz,tempRTC,vBat,vReg"

#endif
