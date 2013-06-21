/*----------------------------------------------------------------------*
 * config.h -- user-definable parameters                                *
 * define logging interval, eeprom characteristics and the log data     *
 * structure here.                                                      *
 *----------------------------------------------------------------------*/

#ifndef config_h
#define config_h

#define LOG_INTERVAL 60        //logging interval in seconds, must be > 0
#define WRAP_MODE false        //true to overwrite oldest data once EEPROM is full, false to stop logging when EEPROM full
#define NBR_EEPROM 2           //NUMBER of EEPROM devices on the I2C bus
#define EEPROM_SIZE 256        //capacity of a SINGLE EEPROM device in KILOBYTES
#define EEPROM_PAGE 256        //EEPROM page size in BYTES

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
    int tempRTC;
    int tempDS;
    int tempDHT;
    int rhDHT;
    int ldr;
    int vBat;
    int vReg;
    byte RFU[2];          //make the log record a multiple of 4 bytes
};

//this line defines the field names and is printed at the beginning of the data when downloading
#define CSV_HEADER "utc,local,tz,tempRTC,tempDS,tempDHT,rhDHT,ldr,vBat,vReg"

#endif

