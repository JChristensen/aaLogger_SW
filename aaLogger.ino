// Double-A DataLogger: A low-power Arduino-based data logger.
// https://github.com/JChristensen/aaLogger_SW
// Copyright (C) 2013-2026 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

// Sketch to log date/time, DS3232 temperature, voltages from
// the A1, A2, A3 pins, and the battery and regulator voltages.

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <DS3232RTC.h>          // https://github.com/JChristensen/DS3232RTC
#include <JC_EEPROM.h>          // https://github.com/JChristensen/JC_EEPROM
#include <OneWire.h>            // https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <Streaming.h>          // https://github.com/janelia-arduino/Streaming
#include <TimeLib.h>            // https://playground.arduino.cc/Code/Time
#include <Timezone.h>           // https://github.com/JChristensen/Timezone
#include <Wire.h>               // https://arduino.cc/en/Reference/Wire
#include "config.h"
#include "defs.h"
#include "logData.h"

// Continental US Time Zones
TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};    // Daylight time = UTC - 5 hours
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};     // Standard time = UTC - 6 hours
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};    // Daylight time = UTC - 6 hours
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};     // Standard time = UTC - 7 hours
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};    // Daylight time = UTC - 7 hours
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};     // Standard time = UTC - 8 hours
Timezone myTZ(EDT, EST);    // use the time change rules for your time zone (or declare new ones)
TimeChangeRule *tcr;        // pointer to the time change rule, use to get TZ abbrev

Button btnStart(START_BUTTON);
Button btnDownload(DWNLD_BUTTON);
DS3232RTC myRTC;

// global variables
int16_t vccBattery, vccRegulator;   // battery and regulator voltages, read in setSystemClock() function
uint8_t nLogBlink;                  // counter for blinking LED when logging a record

// states for the state machine
enum STATES {ENTER_COMMAND, COMMAND, INITIALIZE, LOGGING, POWER_DOWN,
             DOWNLOAD, SET, SET_TIME, SET_INTERVAL} STATE;

void setup()
{
    time_t rtcTime, localTime;

    const uint8_t pinModes[] = {    //initial pin configuration
        INPUT_PULLUP,   // 0    RXD
        INPUT_PULLUP,   // 1    TXD
        OUTPUT,         // 2    peripheral power (RTC, EEPROMs)
        INPUT_PULLUP,   // 3    RTC interrupt
        OUTPUT,         // 4    boost regulator enable
        INPUT_PULLUP,   // 5    download/set button
        INPUT_PULLUP,   // 6    start/init button
        OUTPUT,         // 7    red LED
        OUTPUT,         // 8    green LED
        OUTPUT,         // 9    sensor power enable
        INPUT_PULLUP,   // 10   [SS] unused
        INPUT_PULLUP,   // 11   [MOSI] unused
        INPUT_PULLUP,   // 12   [MISO] unused
        INPUT_PULLUP,   // 13   [SCK] unused
        INPUT_PULLUP,   // A0   unused
        INPUT,          // A1   v1
        INPUT,          // A2   v2
        INPUT,          // A3   v3
        INPUT,          // A4   [SDA] external pullup on board
        INPUT           // A5   [SCL] external pullup on board
    };

    for (uint8_t i=0; i<sizeof(pinModes); i++) {    //configure pins
        pinMode(i, pinModes[i]);
    }
    btnStart.begin();
    btnDownload.begin();
    peripPower(true);                   // peripheral power on
    digitalWrite(SENSOR_POWER, LOW);    // sensor power off
    setSystemClock(CLOCK_8MHZ);
    Serial.begin(BAUD_RATE);
    delay(100);
    Serial << F("\nDouble-A Data Logger\nCompiled " __DATE__ " " __TIME__ "\n");
    Serial << F(__FILE__ "\n");

    myRTC.begin();
    rtcTime = myRTC.get();
    localTime = myTZ.toLocal(rtcTime, &tcr);
    printDateTime(rtcTime, "UTC"); printDateTime(localTime, tcr->abbrev);
    LOGDATA.configChanged(true);
    STATE = ENTER_COMMAND;
    EEEP.begin(JC_EEPROM::twiClock400kHz);
}

void loop()
{
    time_t rtcTime, utc, local, alarmTime;
    static bool redLedState, grnLedState;
    static uint32_t ms, msLast;
    static uint32_t msStateTime;    // time spent in a particular state

    ms = millis();
    switch (STATE)
    {
        case ENTER_COMMAND:         // transition state before entering the COMMAND state
            msStateTime = ms;       // record the time command mode started
            msLast = ms;
            digitalWrite(RED_LED, redLedState = HIGH);
            digitalWrite(GRN_LED, LOW);
            STATE = COMMAND;
            break;

        case COMMAND:
            btnDownload.read();
            btnStart.read();
            if (btnDownload.pressedFor(LONG_PRESS)) {
                STATE = SET;
                digitalWrite(RED_LED, LOW);
                digitalWrite(GRN_LED, grnLedState = HIGH);
                Serial.setTimeout(10000);
                Serial << F("\nSet time or logging interval or exit? [T|t|L|l|cr]\n");
                while (btnDownload.isPressed()) btnDownload.read();
                msStateTime = ms;
            }
            else if (btnStart.pressedFor(LONG_PRESS))
                STATE = INITIALIZE;
            else if (btnDownload.wasReleased())
                STATE = DOWNLOAD;
            else if (btnStart.wasReleased()) {
                if (!LOGDATA.readLogStatus(false)) {    // is there room in the eeprom?
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
                for (uint8_t i=0; i<3; i++) {   // blink the LED to acknowledge
                    digitalWrite(GRN_LED, HIGH);
                    delay(BLIP_ON);
                    digitalWrite(GRN_LED, LOW);
                    delay(BLIP_ON);
                }

                // calculate the first alarm
                rtcTime = myRTC.get();
                alarmTime = rtcTime + (LOGDATA.getLogInterval()) - rtcTime % (LOGDATA.getLogInterval());

                // set RTC alarm to match on hours, minutes, seconds
                myRTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(alarmTime), minute(alarmTime), hour(alarmTime), 0);
                myRTC.alarm(DS3232RTC::ALARM_1);                // clear RTC interrupt flag
                myRTC.alarmInterrupt(DS3232RTC::ALARM_1, true); // enable alarm interrupts

                EICRA = _BV(ISC11);     // interrupt on falling edge
                EIFR = _BV(INTF1);      // clear the interrupt flag (setting ISCnn can cause an interrupt)
                EIMSK = _BV(INT1);      // enable INT1
                gotoSleep(false);       // go to sleep, shut the regulator down
            }
            else if (ms - msStateTime >= STATE_TIMEOUT * 1000UL) {
                STATE = POWER_DOWN;
            }

            // run the LED
            if ((redLedState && ms - msLast >= BLIP_ON) || (!redLedState && ms - msLast >= BLIP_OFF)) {
                msLast = ms;
                digitalWrite(RED_LED, redLedState = !redLedState);
            }
            break;

        case INITIALIZE:
        Serial << endl << F("INITIALIZED") << endl;
           for (uint8_t i=0; i<3; i++) {    // blink both LEDs to acknowledge
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
            // disable RTC alarms so no interrupts are generated
            // there is no exit from this state except a reset
           for (uint8_t i=0; i<5; i++) {        // signal power down
                digitalWrite(RED_LED, HIGH);
                delay(BLIP_ON);
                digitalWrite(RED_LED, LOW);
                delay(BLIP_ON);
            }
            Serial << endl << F("POWER DOWN") << endl;
            myRTC.alarmInterrupt(DS3232RTC::ALARM_1, false);
            myRTC.alarmInterrupt(DS3232RTC::ALARM_2, false);
            EIMSK = 0;              // might as well also disable external interrupts to make absolutely sure
            gotoSleep(false);
            STATE = ENTER_COMMAND;  // should never get here but just in case
            break;

        case DOWNLOAD:
            LOGDATA.download(&myTZ);
            STATE = ENTER_COMMAND;
            break;

        case SET:
            char buf[4];
            Serial.readBytes(buf, 1);
            if (buf[0] == 'T' or buf[0] == 't') {
                STATE = SET_TIME;
                Serial << F("\nEnter UTC time (24-hr clock) as yy,m,d,h,m,s,\n");
            }
            else if (buf[0] == 'L' or buf[0] == 'l') {
                STATE = SET_INTERVAL;
                Serial << F("\nEnter logging interval in seconds:\n");
            }
            else {
                STATE = ENTER_COMMAND;
                Serial << F("Set canceled.\n");
            }
            while (Serial.available() > 0) Serial.read();   // dump any extraneous input
            break;

        case SET_INTERVAL:
            {
                int32_t logInt = Serial.parseInt();
                if (logInt > 0) {
                    LOGDATA.putLogInterval(logInt);
                    LOGDATA.readLogStatus(true);
                }
                else {
                    Serial << F("Log interval not set, must be > 0.\n");
                }
            }
            while (Serial.available() > 0) Serial.read();   // dump any extraneous input
            STATE = ENTER_COMMAND;
            break;

        case SET_TIME:
            // check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
            if (Serial.available() >= 12) {
                tmElements_t tm;
                int y = Serial.parseInt();
                tm.Year = y2kYearToTm(y);    //tmElements_t Year member is an offset from 1970
                tm.Month = Serial.parseInt();
                tm.Day = Serial.parseInt();
                tm.Hour = Serial.parseInt();
                tm.Minute = Serial.parseInt();
                tm.Second = Serial.parseInt();
                utc = makeTime(tm);
                setTime(utc);
                myRTC.set(utc);
                // clear the status register (OSF, BB32KHZ, EN32KHZ are on by default)
                myRTC.writeRTC(DS3232RTC::DS32_STATUS, 0x00);
                local = myTZ.toLocal(utc, &tcr);
                while (Serial.read() >= 0);
                Serial << endl << F("Time set to: ") << endl;
                printDateTime(utc, "UTC");
                printDateTime(local, tcr -> abbrev);
                while (Serial.available() > 0) Serial.read();   // dump any extraneous input
                STATE = ENTER_COMMAND;
            }
            else if (ms - msStateTime >= STATE_TIMEOUT * 1000UL){
                STATE = POWER_DOWN;
            }

            // run the LED
            if ((grnLedState && ms - msLast >= BLIP_ON) || (!redLedState && ms - msLast >= BLIP_OFF)) {
                msLast = ms;
                digitalWrite(GRN_LED, grnLedState = !grnLedState);
            }
            break;
    }
}

// read the sensors, log the data, then sleep.
// when changing the log data structure, the code blocks below with
// comments (1), (2) and (3) will need modification.
// block (3) is optional and can be deleted if desired, doing so will save a little run time and therefore power.
void logSensorData()
{
    time_t rtcTime, alarmTime;
    int16_t tempRTC;
    uint8_t stat;
    int16_t v1, v2, v3;

    rtcTime = myRTC.get();

    { /*---- (1) READ SENSORS ----*/
        tempRTC = myRTC.temperature() * 9 / 2 + 320;    // °F * 10
        digitalWrite(SENSOR_POWER, HIGH);
        v1 = analogRead(A1);
        v2 = analogRead(A2);
        v3 = analogRead(A3);
        v1 = analogRead(A1);
        v2 = analogRead(A2);
        v3 = analogRead(A3);
        digitalWrite(SENSOR_POWER, LOW);
    }

    { /*---- (2) SAVE SENSOR DATA ----*/
        LOGDATA.fields.timestamp = rtcTime;
        LOGDATA.fields.tempRTC = tempRTC;
        LOGDATA.fields.vBat = vccBattery;
        LOGDATA.fields.vReg = vccRegulator;
        LOGDATA.fields.v1 = v1;
        LOGDATA.fields.v2 = v2;
        LOGDATA.fields.v3 = v3;
    }

    stat = LOGDATA.write();
    if (stat == EEPROM_FULL_ERR) {
        Serial << F("EEPROM FULL") << endl;
        STATE = POWER_DOWN;
        return;
    }
    else if (stat == JC_EEPROM::EEPROM_ADDR_ERR) {
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
        Serial << F(", ") << tempRTC << F(", ");
        Serial << vccBattery << F(", ") << vccRegulator << F(", ")
               << (long)v1 * vccRegulator / 1024 << F(", ")
               << (long)v2 * vccRegulator / 1024 << F(", ")
               << (long)v3 * vccRegulator / 1024 << endl;
    }

    // calculate and set the next alarm
    alarmTime = rtcTime + (LOGDATA.getLogInterval());
    myRTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(alarmTime), minute(alarmTime), hour(alarmTime), 0);
    myRTC.alarm(DS3232RTC::ALARM_1);    // clear RTC interrupt flag

    // blink LED to indicate record logged
    if (nLogBlink) {
        --nLogBlink;
        digitalWrite(GRN_LED, HIGH);
        delay(LOG_BLINK);
        digitalWrite(GRN_LED, LOW);
    }
    gotoSleep(false);   // go back to sleep, shut the regulator down
}

void gotoSleep(bool enableRegulator)
{
    uint8_t adcsra, mcucr1, mcucr2;

    Serial.flush();
    Serial.end();
    peripPower(false);              // peripheral power off
    digitalWrite(RED_LED, LOW);     // LEDs off
    digitalWrite(GRN_LED, LOW);
    pinMode(SCL, INPUT);            // tri-state the i2c bus
    pinMode(SDA, INPUT);
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    if (!enableRegulator) {
        digitalWrite(SENSOR_POWER, LOW);    // sensor power off
        setSystemClock(CLOCK_1MHZ);
    }
    adcsra = ADCSRA;        // save the ADC Control and Status Register A
    ADCSRA = 0;             // disable ADC
    // disable brown-out detector while MCU sleeps, must sleep within four clock cycles
    cli();
    mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);
    mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;
    sei();                      // ensure interrupts enabled so we can wake up again
    sleep_cpu();                // go to sleep
    sleep_disable();            // wake up here
    if (!enableRegulator) setSystemClock(CLOCK_8MHZ);
    ADCSRA = adcsra;            // restore ADCSRA
    Serial.begin(BAUD_RATE);
    peripPower(true);           // peripheral power on
    delay(1);                   // a little ramp-up time
}

// interrupt from the RTC alarm. don't need to do anything, it's just to wake the MCU.
ISR(INT1_vect) {}

// enables the boost regulator to provide higher voltage and increases the system clock frequency,
// or decreases the system clock frequency and disables the regulator to run on direct battery voltage.
void setSystemClock(uint8_t clkpr)
{
    if (clkpr == CLOCK_8MHZ) {
        ADCSRA = 0x84;          // adjust the ADC prescaler for slower system clock
        vccBattery = readVcc();
        digitalWrite(BOOST_REGULATOR, HIGH);
        delay(1);               // actually 8ms because the clock is 1MHz at this point
        vccRegulator = readVcc();
    }

    cli();
    CLKPR = _BV(CLKPCE);        // set the clock prescaler change enable bit
    CLKPR = clkpr;
    sei();

    if (clkpr == CLOCK_1MHZ) {
        ADCSRA = 0x87;           // adjust the ADC prescaler for faster system clock
        digitalWrite(BOOST_REGULATOR, LOW);
        delay(1);                // actually 8ms because the clock is 1MHz at this point
    }
}

// turn peripheral (rtc, eeprom) power on or off,
// using direct port manipulation for fastest transition.
// The PD2 pin powers the peripherals (Arduino pin D2).
void peripPower(bool enable)
{
    if (enable) {               // turn power on
        PORTD |= _BV(PORTD2);   // input pullup is transition state
        DDRD |= _BV(DDD2);      // output high
    }
    else {                      // turn power off
        DDRD &= ~_BV(DDD2);     // input pullup is transition state
        PORTD &= ~_BV(PORTD2);  // turn off pullup for tri-state/hi-z
    }
}
