/*----------------------------------------------------------------------*
 * config.h -- user-definable parameters                                *
 * define logging interval, eeprom characteristics and the log data     *
 * structure here.                                                      *
 *----------------------------------------------------------------------*/

#ifndef config_h
#define config_h

#define LOG_INTERVAL 1    //logging interval in minutes, must be >= 1 and <= 60
#define NBR_EEPROM 1      //number of EEPROM devices on the I2C bus
#define EEPROM_SIZE 32    //capacity of a SINGLE EEPROM device in KILOBYTES
#define EEPROM_PAGE 64    //eeprom page size in BYTES
#define WRAP_MODE true   //true to overwrite oldest data once eeprom is full, false to stop logging when eeprom full

/*----------------------------------------------------------------------*
 * The struct below defines the log data. When modifying the struct,    *
 * also change the logData::download() function in logData.cpp and the  *
 * logSensorData() function in the main module accordingly.             *
 *                                                                      *
 * When using M24M02 EEPROMs, the size of the struct should be a        *
 * multiple of four bytes if at all possible. This will minimize the    *
 * number of write cycles and therefore maximize EEPROM endurance.      *
 * Pad the struct out with a byte array, e.g. byte RFU[2]; if needed.   *
 *----------------------------------------------------------------------*/
struct logData_t {
    unsigned long timestamp;
    int sensorTemp;
    int rtcTemp;
    int batteryVoltage;
    int regulatorVoltage;
};

//this line defines the field names and is printed at the beginning of the data when downloading
#define CSV_HEADER "utc,local,tz,sensorTemp,rtcTemp,batteryVoltage,regulatorVoltage"

#endif

