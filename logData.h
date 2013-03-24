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
#define RTC_TYPE 79412        //set to 79412 for MCP79412 or 3232 for DS3232 only.

#include <extEEPROM.h>        //http://github.com/JChristensen/extEEPROM
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Timezone.h>         //http://github.com/JChristensen/Timezone

#define DEBUG_MODE 0
#define RED_LED 6             //duplicated from main module
#define GRN_LED 7
#define BLIP_ON 100
#if RTC_TYPE == 79412
#define RTC_RAM_STATUS 0x00   //address in the RTC SRAM to keep log status
#else
#define RTC_RAM_STATUS 0x14
#endif

/*----------------------------------------------------------------------*
 * The struct below defines the log data. The size of the struct in     *
 * bytes should be a power of two, and must be less than or equal to    *
 * the EEPROM page size. Use a byte array as the last element in the    *
 * struct as shown below to make the size a power of two if needed.     *
 *                                                                      *
 * After modifying the struct, also change the logData::download()      *
 * and the logSensorData() functions accordingly.                       *
 *                                                                      *
 * The maximum struct size supported by the current code is 16 bytes    *
 * due to the Arduino Wire library buffer size limitation. Increasing   *
 * the limit to something larger (but still a power of two) should be   *
 * fairly straightforward; it would just require multiple reads and     *
 * writes for each log record.                                          *
 *                                                                      *
 * Removing the power-of-two restriction would be more difficult, as    *
 * EEPROM page and device spanning would have to be considered.         *
 *----------------------------------------------------------------------*/
struct logData_t {
    unsigned long timestamp;    // 4 bytes
    int sensorTemp;             // + 2 = 6 bytes
    int rtcTemp;                // + 2 = 8
    int batteryVoltage;         // + 2 = 10
    int regulatorVoltage;       // + 2 = 12
    byte RFU[4];                // + 4 = 16 (reserved for future use)
};

class logData
{
    public:
        logData(unsigned long eepromSize, boolean wrapWhenFull);
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
        unsigned long _eepromSize;        //EEPROM capacity in bytes (total for all EEPROM devices combined)
        static const byte _logRecSize = sizeof(logData_t);
        boolean _wrapWhenFull;            //true: logging continues when EEPROM is full, next record replacing the oldest
                                          //false: logging stops at the top EEPROM memory address
        unsigned long _nextRecord;        //EEPROM address where the next log data record will be written
        boolean _eepromFull;
        unsigned long _readRecord;        //for reading/downloading
    
        union {                           //logging status data persisted in RTC SRAM (battery-backed)
            struct {
                unsigned long eepromSize; //copy of _eepromSize
                byte recSize;             //copy of _logRecSize
                boolean wrap;             //copy of _wrapWhenFull
                unsigned long next;       //copy of _nextRecord
                boolean full;             //copy of _eepromFull
            };
            byte bytes[11];
        } logStatus;
    
};

extern logData LOGDATA;

#endif

