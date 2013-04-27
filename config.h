/*----------------------------------------------------------------------*
 * config.h -- user-definable parameters                                *
 * define logging interval, eeprom characteristics and the log data     *
 * structure here.                                                      *
 *----------------------------------------------------------------------*/

#ifndef config_h
#define config_h

#define LOG_INTERVAL 1    //logging interval in minutes, must be >= 1 and <= 60
#define NBR_EEPROM 2      //number of EEPROM devices on the I2C bus
#define EEPROM_SIZE 256   //capacity of a SINGLE EEPROM device in KILOBYTES
#define EEPROM_PAGE 256   //EEPROM page size in BYTES
#define WRAP_MODE false   //true to overwrite oldest data once EEPROM is full, false to stop logging when EEPROM full

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
    int tempSensor;
    int tempRTC;
    int ldr1;
    int vBat1;
    int vBat2;
    int vReg;
};

//this line defines the field names and is printed at the beginning of the data when downloading
#define CSV_HEADER "utc,local,tz,tempSensor,tempRTC,ldr,vBat1,vBat2,vReg"

#endif

