//_main.ino - Double-A DataLogger main module

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Button.h>           //http://github.com/JChristensen/Button
#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <extEEPROM.h>        //http://github.com/JChristensen/extEEPROM
#include <OneWire.h>          //http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Timezone.h>         //http://github.com/JChristensen/Timezone
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire
#include "config.h"
#include "defs.h"
#include "logData.h"

//Continental US Time Zones
TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};    //Daylight time = UTC - 5 hours
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};     //Standard time = UTC - 6 hours
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};    //Daylight time = UTC - 6 hours
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};     //Standard time = UTC - 7 hours
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(EDT, EST);    //use the time change rules for your time zone (or declare new ones)
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

Button btnStart(START_BUTTON, true, true, DEBOUNCE_MS);
Button btnDownload(DWNLD_BUTTON, true, true, DEBOUNCE_MS);

//global variables
int vBat, vccBattery, vccRegulator;   //battery and regulator voltages, read in setSystemClock() function
byte nLogBlink;                       //counter for blinking LED when logging a record

//states for the state machine
enum STATES {ENTER_COMMAND, COMMAND, INITIALIZE, LOGGING, POWER_DOWN, DOWNLOAD, SET_TIME} STATE;

void setup(void)
{
    time_t rtcTime, localTime;

    const uint8_t pinModes[] = {        //initial pin configuration
        INPUT_PULLUP,    //0    RXD
        INPUT_PULLUP,    //1    TXD
        OUTPUT,          //2    peripheral power (RTC, EEPROMs)
        INPUT_PULLUP,    //3    RTC interrupt
        OUTPUT,          //4    boost regulator enable
        INPUT_PULLUP,    //5    download/set button
        INPUT_PULLUP,    //6    start/init button
        OUTPUT,          //7    red LED
        OUTPUT,          //8    green LED
        OUTPUT,          //9    sensor power enable
        INPUT_PULLUP,    //10   [SS] unused
        INPUT_PULLUP,    //11   [MOSI] unused
        INPUT_PULLUP,    //12   [MISO] unused
        INPUT_PULLUP,    //13   [SCK] unused
        INPUT_PULLUP,    //A0   unused
        INPUT_PULLUP,    //A1   unused
        INPUT_PULLUP,    //A2   unused
        INPUT_PULLUP,    //A3   unused
        INPUT,           //A4   [SDA] external pullup on board
        INPUT            //A5   [SCL] external pullup on board
    };

    for (uint8_t i=0; i<sizeof(pinModes); i++) {    //configure pins
        pinMode(i, pinModes[i]);
    }
    peripPower(true);                 //peripheral power on
    PORTB &= ~_BV(PORTB1);            //sensor power off
    setSystemClock(CLOCK_8MHZ);
    Serial.begin(BAUD_RATE);
    
    rtcTime = RTC.get();
    localTime = myTZ.toLocal(rtcTime, &tcr);
    Serial << endl << F("Double-A Data Logger SW-v") << _DEC(SOFTWARE_VERSION) << endl;
    printDateTime(rtcTime, "UTC"); printDateTime(localTime, tcr -> abbrev);
    LOGDATA.configChanged(true);
    STATE = ENTER_COMMAND;
    TWBR = 2;    //sets I2C SCL to 400kHz SCL (assuming 8MHz system clock)
}

void loop(void)
{
    time_t rtcTime, utc, local, alarmTime;
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
                
                //calculate the first alarm
                rtcTime = RTC.get();
                alarmTime = rtcTime + (LOG_INTERVAL) - rtcTime % (LOG_INTERVAL);
            
                //set RTC alarm to match on hours, minutes, seconds
                RTC.setAlarm(ALM1_MATCH_HOURS, second(alarmTime), minute(alarmTime), hour(alarmTime), 0);
                RTC.alarm(ALARM_1);                   //clear RTC interrupt flag
                RTC.alarmInterrupt(ALARM_1, true);    //enable alarm interrupts
                
                EICRA = _BV(ISC11);               //interrupt on falling edge
                EIFR = _BV(INTF1);                //clear the interrupt flag (setting ISCnn can cause an interrupt)
                EIMSK = _BV(INT1);                //enable INT1
                gotoSleep(false);                 //go to sleep, shut the regulator down
            }
            else if (ms - msStateTime >= STATE_TIMEOUT * 1000UL) {
                STATE = POWER_DOWN;
            }
            
            //run the LED
            if ((redLedState && ms - msLast >= BLIP_ON) || (!redLedState && ms - msLast >= BLIP_OFF)) {
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
            RTC.alarmInterrupt(ALARM_1, false);
            RTC.alarmInterrupt(ALARM_2, false);
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
                RTC.writeRTC(RTC_STATUS, 0x00);      //clear the status register (OSF, BB32KHZ, EN32KHZ are on by default)
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
            if ((grnLedState && ms - msLast >= BLIP_ON) || (!redLedState && ms - msLast >= BLIP_OFF)) {
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
    time_t rtcTime, alarmTime;
    int tempRTC;
    byte stat;
    int tempDS;                         //DS18B20 sensor temperature (fahrenheit times 10)
    int ldr;

    rtcTime = RTC.get();

    { /*---- (1) READ SENSORS ----*/
        tempRTC = RTC.temperature() * 9 / 2 + 320;
        PORTB |= _BV(PORTB1);                           //sensor power on
        if (!readDS18B20(&tempDS)) tempDS = -9999;      //use -999.9F to indicate CRC error
        ldr = analogRead(LDR);
        PORTB &= ~_BV(PORTB1);                          //sensor power off
    }

    { /*---- (2) SAVE SENSOR DATA ----*/
        LOGDATA.fields.timestamp = rtcTime;
        LOGDATA.fields.tempRTC = tempRTC;
        LOGDATA.fields.tempDS = tempDS;
        LOGDATA.fields.ldr = ldr;
        LOGDATA.fields.vBat = vccBattery;
        LOGDATA.fields.vReg = vccRegulator;
    }

    stat = LOGDATA.write();
    if (stat == EEPROM_FULL_ERR) {
        Serial << F("EEPROM FULL") << endl;
        STATE = POWER_DOWN;
        return;
    }
    else if (stat == EEPROM_ADDR_ERR) {
        Serial << F("EEPROM ADDRESS ERROR") << endl;
        STATE = POWER_DOWN;
        return;
    }
    else if (stat != 0) {
        Serial << F("EEPROM WRITE ERROR ") << _DEC(stat) << endl;
        STATE = POWER_DOWN;
        return;
    }

    { /*---- (3) PRINT DATA TO SERIAL MONITOR ----*/
        printTime(rtcTime); printDate(rtcTime);
        Serial << F(", ") << tempRTC;
        Serial << F(", ") << tempDS <<  F(", ") << ldr;
        Serial  << F(", ") << vccBattery << F(", ") << vccRegulator << endl;
    }

    //calculate and set the next alarm
    alarmTime = rtcTime + (LOG_INTERVAL);
    RTC.setAlarm(ALM1_MATCH_HOURS, second(alarmTime), minute(alarmTime), hour(alarmTime), 0);
    RTC.alarm(ALARM_1);               //clear RTC interrupt flag

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
    peripPower(false);                 //peripheral power off
    digitalWrite(RED_LED, LOW);        //LEDs off
    digitalWrite(GRN_LED, LOW);
    pinMode(SCL, INPUT);               //tri-state the i2c bus   
    pinMode(SDA, INPUT);
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    if (!enableRegulator) {
        PORTB &= ~_BV(PORTB1);         //sensor power off
        setSystemClock(CLOCK_1MHZ);
    }
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
    peripPower(true);              //peripheral power on
    delay(1);                      //a little ramp-up time
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
    CLKPR = _BV(CLKPCE);                     //set the clock prescaler change enable bit
    CLKPR = clkpr;
    sei();
    
    if (clkpr == CLOCK_1MHZ) {
        ADCSRA = 0x87;                       //adjust the ADC prescaler for faster system clock
        digitalWrite(BOOST_REGULATOR, LOW);
        delay(1);                            //actually 8ms because the clock is 1MHz at this point
    }
}

//turn peripheral (rtc, eeprom) power on or off,
//using direct port manipulation for fastest transition.
//The PD2 pin powers the peripherals (Arduino pin D2).
void peripPower(boolean enable)
{
    if (enable) {                   //turn power on
        PORTD |= _BV(PORTD2);       //input pullup is transition state
        DDRD |= _BV(DDD2);          //output high
    }
    else {                          //turn power off
        DDRD &= ~_BV(DDD2);         //input pullup is transistion state
        PORTD &= ~_BV(PORTD2);      //turn off pullup for tri-state/hi-z
    }
}
