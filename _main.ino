//_main.ino - Double-A DataLogger main module

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Button.h>           //http://github.com/JChristensen/Button
#include <extEEPROM.h>        //http://github.com/JChristensen/extEEPROM
#include <OneWire.h>          //http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Timezone.h>         //http://github.com/JChristensen/Timezone
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire
#include "config.h"
#include "defs.h"
#include "logData.h"

//select RTC by commenting one of the next two lines, and setting the #define accordingly.
//make similar changes in the logData.h file also.
//#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <MCP79412RTC.h>      //http://github.com/JChristensen/MCP79412RTC

//Continental US Time Zones
TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};    //Daylight time = UTC - 5 hours
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};     //Standard time = UTC - 6 hours
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};    //Daylight time = UTC - 6 hours
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};     //Standard time = UTC - 7 hours
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(EDT, EST);                                    //use the time change rules for your time zone (or declare new ones)
TimeChangeRule *tcr;                  //pointer to the time change rule, use to get TZ abbrev

Button btnStart(START_BUTTON, true, true, DEBOUNCE_MS);
Button btnDownload(DWNLD_BUTTON, true, true, DEBOUNCE_MS);

//global variables
int vccBattery, vccRegulator;         //battery and regulator voltages
byte nLogBlink;                       //counter for blinking LED when logging a record

//states for the state machine
enum STATES {ENTER_COMMAND, COMMAND, INITIALIZE, LOGGING, POWER_DOWN, DOWNLOAD, SET_TIME} STATE;

#if RTC_TYPE == 79412
tmElements_t tm;
time_t alarmTime;
int rtcTemp;                          //use as a counter with mcp79412 rtc
#endif

void setup(void)
{
    time_t rtcTime, localTime;

    const uint8_t pinModes[] = {        //initial pin configuration
        INPUT_PULLUP,    //0    RXD
        INPUT_PULLUP,    //1    TXD
        INPUT_PULLUP,    //2    unused
        INPUT_PULLUP,    //3    RTC interrupt
        INPUT_PULLUP,    //4    unused
        INPUT_PULLUP,    //5    unused
        OUTPUT,          //6    red LED
        OUTPUT,          //7    green LED
        OUTPUT,          //8    spare LED
        INPUT,           //9    DS18B20 data line (external pullup)
        OUTPUT,          //10   DS18B20 ground
        OUTPUT,          //11   regulator control
        INPUT_PULLUP,    //12   unused
        OUTPUT,          //13   builtin LED
        INPUT_PULLUP,    //A0   unused
        OUTPUT,          //A1   peripheral power (RTC, EEPROMs)
        INPUT_PULLUP,    //A2   start/init button
        INPUT_PULLUP,    //A3   download/set button
        INPUT,           //A4   SDA (external pullup)
        INPUT            //A5   SCL (external pullup)
    };

    for (uint8_t i=0; i<sizeof(pinModes); i++) {    //configure pins
        pinMode(i, pinModes[i]);
    }
    digitalWrite(PERIP_POWER, HIGH);  //peripheral power on
    setSystemClock(CLOCK_8MHZ);
    Serial.begin(BAUD_RATE);

    rtcTime = RTC.get();
    localTime = myTZ.toLocal(rtcTime, &tcr);
    Serial << endl << F("Double-A Data Logger SW-v") << _DEC(SOFTWARE_VERSION) << endl;
    printDateTime(rtcTime, "UTC"); printDateTime(localTime, tcr -> abbrev);
    LOGDATA.configChanged(true);
    STATE = ENTER_COMMAND;
    #if DEBUG_MODE == 1
    dumpRTC(32);
    #endif
}

void loop(void)
{
    uint8_t curMin, alarmMin;
    time_t rtcTime, utc, local;
    static boolean redLedState, grnLedState;
    static unsigned long ms, msLast;
    static unsigned long msStateTime;        //time spent in a particular state
    
    ms = millis();
    switch (STATE)
    {
        case ENTER_COMMAND:                  //transition state before entering the COMMAND state
            msStateTime = ms;                //record the time command mode started
            digitalWrite(RED_LED, redLedState = HIGH);
            digitalWrite(GRN_LED, LOW);
            STATE = COMMAND;
            break;
            
        case COMMAND:
            btnDownload.read();
            btnStart.read();
            if (btnDownload.pressedFor(LONG_PRESS)) {
                STATE = SET_TIME;
                digitalWrite(RED_LED, LOW);
                digitalWrite(GRN_LED, grnLedState = HIGH);
                Serial << endl << F("Enter Unix Epoch Time (UTC), e.g. from http://www.epochconverter.com/") << endl;
                while (btnDownload.isPressed()) btnDownload.read();
                msStateTime = ms;
            }
            else if (btnStart.pressedFor(LONG_PRESS))
                STATE = INITIALIZE;
            else if (btnDownload.wasReleased())
                STATE = DOWNLOAD;
            else if (btnStart.wasReleased()) {
                if (!LOGDATA.readLogStatus(false)) {        //is there room in the eeprom?
                    Serial << F("EEPROM FULL") << endl;
                    STATE = ENTER_COMMAND;
                    break;
                }
                else if (LOGDATA.configChanged(false)) {
                    STATE = ENTER_COMMAND;
                    break;
                }
                STATE = LOGGING;
                nLogBlink = N_LOG_BLINK;
                Serial << endl << F("LOGGING") << endl;
                digitalWrite(RED_LED, LOW);
                for (uint8_t i=0; i<3; i++) {     //blink the LED to acknowledge
                    digitalWrite(GRN_LED, HIGH);
                    delay(BLIP_ON);
                    digitalWrite(GRN_LED, LOW);
                    delay(BLIP_ON);
                }
                
                //calculate the minute for the first alarm
                rtcTime = RTC.get();
                curMin = minute(rtcTime);
                alarmMin = (curMin - curMin % LOG_INTERVAL + LOG_INTERVAL) % 60;
                #if DEBUG_MODE == 1
                Serial << F("First alarm=") << _DEC(alarmMin) << endl;
                #endif
            
                //set the alarm
                #if RTC_TYPE == 79412
                breakTime(rtcTime, tm);
                tm.Minute = alarmMin;
                tm.Second = 0;
                alarmTime = makeTime(tm);
                //set RTC alarm 0, enable to match on minutes
                RTC.setAlarm(ALARM_0, alarmTime);
                RTC.alarm(ALARM_0);               //clear the alarm flag
                RTC.enableAlarm(ALARM_0, ALM_MATCH_MINUTES);
                #if DEBUG_MODE == 1
                dumpRTC(32);
                #endif
                #else    
                RTC.setAlarm( ALM2_MATCH_MINUTES, alarmMin, 0, 0);    //set RTC alarm 2 to match on minutes
                RTC.alarm(ALARM_2);                   //clear RTC interrupt flag
                RTC.alarmInterrupt(ALARM_2, true);    //enable alarm 2 interrupts
                #if DEBUG_MODE == 1
                dumpRtcRegisters();
                #endif
                #endif
                
                EICRA = _BV(ISC11);               //interrupt on falling edge
                EIFR = _BV(INTF1);                //clear the interrupt flag (setting ISCnn can cause an interrupt)
                EIMSK = _BV(INT1);                //enable INT1
                gotoSleep(false);                 //go to sleep, shut the regulator down
            }
            else if (ms - msStateTime >= STATE_TIMEOUT * 1000UL){
                STATE = POWER_DOWN;
            }
            
            //run the LED
            if ((redLedState && ms - msLast > BLIP_ON) || (!redLedState && ms - msLast > BLIP_OFF)) {
                msLast = ms;
                digitalWrite(RED_LED, redLedState = !redLedState);
            }
            break;
        
        case INITIALIZE:
        Serial << endl << F("INITIALIZED") << endl;
           for (uint8_t i=0; i<3; i++) {     //blink both LEDs to acknowledge
                digitalWrite(RED_LED, HIGH);
                digitalWrite(GRN_LED, HIGH);
                delay(BLIP_ON);
                digitalWrite(RED_LED, LOW);
                digitalWrite(GRN_LED, LOW);
                delay(BLIP_ON);
            }
            while (btnStart.isPressed()) btnStart.read();
            LOGDATA.initialize();
            LOGDATA.readLogStatus(true);
            STATE = ENTER_COMMAND;
            createData(5000);
            break;
            
        case LOGGING:
            logSensorData();
            break;
            
        case POWER_DOWN:
            //disable RTC alarms so no interrupts are generated
            //there is no exit from this state except a reset
           for (uint8_t i=0; i<5; i++) {         //signal power down
                digitalWrite(RED_LED, HIGH);
                delay(BLIP_ON);
                digitalWrite(RED_LED, LOW);
                delay(BLIP_ON);
            }
            Serial << endl << F("POWER DOWN") << endl;
            #if RTC_TYPE == 79412
            RTC.enableAlarm(ALARM_0, ALM_DISABLE);
            RTC.enableAlarm(ALARM_1, ALM_DISABLE);
            #else
            RTC.alarmInterrupt(ALARM_1, false);
            RTC.alarmInterrupt(ALARM_2, false);
            #endif
            EIMSK = 0;                //might as well also disable external interrupts to make absolutely sure
            gotoSleep(false);
            STATE = ENTER_COMMAND;    //should never get here but just in case
            break;
            
        case DOWNLOAD:
            LOGDATA.download(&myTZ);
            STATE = ENTER_COMMAND;
            break;
            
        case SET_TIME:
            if (Serial.available() >= 10) {
                utc = Serial.parseInt();
                setTime(utc);
                RTC.set(utc);
                local = myTZ.toLocal(utc, &tcr);
                while (Serial.read() >= 0);
                Serial << endl << F("Time set to: ") << endl;
                printDateTime(utc, "UTC");
                printDateTime(local, tcr -> abbrev);
                STATE = ENTER_COMMAND;
            }
            else if (ms - msStateTime >= STATE_TIMEOUT * 1000UL){
                STATE = POWER_DOWN;
            }

            //run the LED
            if ((grnLedState && ms - msLast > BLIP_ON) || (!redLedState && ms - msLast > BLIP_OFF)) {
                msLast = ms;
                digitalWrite(GRN_LED, grnLedState = !grnLedState);
            }
            break;
    }
}

//read the sensors, log the data, then sleep.
//when changing the log data structure, the code blocks below with
//comments (1), (2) and (3) will need modification.
//block (3) is optional and can be deleted if desired, doing so will save a little run time and therefore power.
void logSensorData(void)
{
    time_t rtcTime;
    uint8_t curMin, alarmMin;
    int tF10;                         //temperature in fahrenheit times 10
    boolean validTemp;
    #if RTC_TYPE == 3232
    int rtcTemp;                      //temperature from RTC times 10
    #endif
    
    digitalWrite(PERIP_POWER, HIGH);  //peripheral power on
    delay(1);                         //a little ramp-up time
    rtcTime = RTC.get();

    { /*---- (1) READ SENSORS ----*/
        validTemp = readDS18B20(&tF10);
        #if RTC_TYPE == 79412
        ++rtcTemp;
        #else
        rtcTemp = RTC.temperature() * 9 / 2 + 320;
        #endif
    }

    { /*---- (2) SAVE SENSOR DATA ----*/
        LOGDATA.fields.timestamp = rtcTime;
        LOGDATA.fields.sensorTemp = tF10;
        LOGDATA.fields.rtcTemp = rtcTemp;
        LOGDATA.fields.batteryVoltage = vccBattery;
        LOGDATA.fields.regulatorVoltage = vccRegulator;
    }

    if (!LOGDATA.write()) {
        #if DEBUG_MODE == 1
        Serial << F("EEPROM FULL") << endl;
        #endif
        STATE = POWER_DOWN;
        return;
    }

    { /*---- (3) PRINT DATA TO SERIAL MONITOR ----*/
        printTime(rtcTime); printDate(rtcTime);
        if (validTemp) Serial << ", " << tF10 / 10 << '.' << tF10 % 10 << " F";
        Serial << ", RTC " << rtcTemp / 10 << '.' << rtcTemp % 10 << " F";
        Serial << F(", Bat ") << vccBattery << F(" mV, Reg ") << vccRegulator << F(" mV") << endl;
    }

    //calculate the minute for the next alarm
    curMin = minute(rtcTime);
    alarmMin = (curMin + LOG_INTERVAL) % 60;
    #if DEBUG_MODE == 1
    Serial << F("Next alarm=") << _DEC(alarmMin) << endl;
    #endif
    
    //set the alarm
    #if RTC_TYPE == 79412
    breakTime(rtcTime, tm);
    tm.Minute = alarmMin;
    tm.Second = 0;
    alarmTime = makeTime(tm);
    RTC.setAlarm(ALARM_0, alarmTime);
    RTC.alarm(ALARM_0);               //clear the alarm flag
    RTC.enableAlarm(ALARM_0, ALM_MATCH_MINUTES);
    #if DEBUG_MODE == 1
    //dumpRTC(32);
    #endif
    #else
    RTC.setAlarm( ALM2_MATCH_MINUTES, alarmMin, 0, 0);
    #if DEBUG_MODE == 1
    dumpRtcRegisters();
    #endif
    RTC.alarm(ALARM_2);               //clear RTC interrupt flag
    #endif

    //blink LED to indicate record logged
    if (nLogBlink) {
        --nLogBlink;
        digitalWrite(GRN_LED, HIGH);
        delay(LOG_BLINK);
        digitalWrite(GRN_LED, LOW);
    }
    
    gotoSleep(false);                 //go back to sleep, shut the regulator down
}

void gotoSleep(boolean enableRegulator)
{
    uint8_t adcsra, mcucr1, mcucr2;
 
    Serial.flush();
    Serial.end();
    digitalWrite(PERIP_POWER, LOW);    //peripheral power off
    pinMode(PERIP_POWER, INPUT);
    digitalWrite(RED_LED, LOW);        //LEDs off
    digitalWrite(GRN_LED, LOW);
    pinMode(SCL, INPUT);               //tri-state the i2c bus   
    pinMode(SDA, INPUT);
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    if (!enableRegulator) setSystemClock(CLOCK_1MHZ);
    adcsra = ADCSRA;               //save the ADC Control and Status Register A
    ADCSRA = 0;                    //disable ADC
    //disable brown-out detector while MCU sleeps, must sleep within four clock cycles
    cli();
    mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);
    mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;
    sei();                         //ensure interrupts enabled so we can wake up again
    sleep_cpu();                   //go to sleep
    sleep_disable();               //wake up here
    if (!enableRegulator) setSystemClock(CLOCK_8MHZ);
    ADCSRA = adcsra;               //restore ADCSRA    
    Serial.begin(BAUD_RATE);
    pinMode(PERIP_POWER, OUTPUT);
}

//interrupt from the RTC alarm. don't need to do anything, it's just to wake the MCU.
ISR(INT1_vect)
{
}

//enables the boost regulator to provide higher voltage and increases the system clock frequency,
//or decreases the system clock frequency and disables the regulator to run on direct battery voltage.
void setSystemClock(uint8_t clkpr)
{
    if (clkpr == CLOCK_8MHZ) {
        ADCSRA = 0x84;                       //adjust the ADC prescaler for slower system clock
        vccBattery = readVcc();
        digitalWrite(BOOST_REGULATOR, HIGH);
        delay(1);                            //actually 8ms because the clock is 1MHz at this point
        vccRegulator = readVcc();
    }

    cli();
    CLKPR = _BV(CLKPCE);        //set the clock prescaler change enable bit
    CLKPR = clkpr;
    sei();
    
    if (clkpr == CLOCK_1MHZ) {
        ADCSRA = 0x87;                       //adjust the ADC prescaler for faster system clock
        digitalWrite(BOOST_REGULATOR, LOW);
        delay(1);                            //actually 8ms because the clock is 1MHz at this point
    }
}

