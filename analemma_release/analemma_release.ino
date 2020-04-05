#include <LowPower.h>
#include <Time.h>
#include <TimeLib.h>

#include <Servo.h>

#include <Wire.h>
#include <DS3231.h>
#include <EEPROM.h>

// Analemmas:
// #1
// 10:00, 11:00, 12:00, 13:00, 14:00  
// brightening: 5:00 - 7:00, 19:00 - 21:00
// + soliticies + equinoxes

// #2 One big (5, 15 minutes?), reversed

// #3 several with longer exposition, 10 minutes?

// #4 connected: 13:01, 13:32, 14:03, 14:34, 15:05, 15:36, 16:07, 16:38

// TODO:
// Use hardware serial for GPS instead of software
// Use lib for finding timezone
// Add time adjusment from gps? meh probably
// dt.unixtime returns wrong value
// Add hatch event every month
// Add hatch event every week
// Handle leap days in yearly event
// Add schedule validation

// --------------   SERVO  ------------------------
Servo motor;
enum Toggle {
  Close = 0,
  Open = 1
};

// ----------------- SCHEDULE  -------------------
struct HatchEvent {
  HatchEvent() {
    openTime = -1;
    closeTime = -1;  
  }
  
  HatchEvent(uint8_t hour, uint8_t minute, uint32_t openForSeconds) {
    time_t currentTime = now();
    TimeElements currentEl;
    breakTime(currentTime, currentEl);

    openTime = makeTime(TimeElements{
      .Second = 0,
      .Minute = minute,
      .Hour = hour,
      .Wday = dowInvalid, // Unused
      .Day = currentEl.Day,
      .Month = currentEl.Month,
      .Year = currentEl.Year
    });

    // Event already passed
    if (currentTime >= openTime) {
      // Schedule for tomorrow
      openTime += SECS_PER_DAY;
    }

    closeTime = openTime + openForSeconds;
  }
  
  HatchEvent(uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint32_t openForSeconds) {
    time_t currentTime = now();
    TimeElements currentEl;
    breakTime(currentTime, currentEl);

    openTime = makeTime(TimeElements{
      .Second = 0,
      .Minute = minute,
      .Hour = hour,
      .Wday = dowInvalid, // Unused
      .Day = day,
      .Month = month,
      .Year = currentEl.Year
    });

    // Event already passed
    if (currentTime >= openTime) {
      // Schedule for next year
      // TODO: leap years?
      openTime += SECS_PER_YEAR;
    }

    closeTime = openTime + openForSeconds;
  }
  
  time_t openTime;
  time_t closeTime;
};

HatchEvent nextEvent;

// --------------   TIME   ------------------------
const int offset = 1;   // Central European Time
//const int offset = -5;  // Eastern Standard Time (USA)
//const int offset = -4;  // Eastern Daylight Time (USA)
//const int offset = -8;  // Pacific Standard Time (USA)
//const int offset = -7;  // Pacific Daylight Time (USA)
//time_t prevDisplay = 0; // when the digital clock was displayed

DS3231 clock;
RTCDateTime dt;
bool isAlarm = false;

void refreshDate() {
  dt = clock.getDateTime();
  
  setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
}

// --------------------------------------------------- Servo ----------------------------------------
void toggleMotor(Toggle toggle) {  
  // Step and delay are set to motor opreate quietly
  static const int STEP = 1;
  static const int DELAY = 40;

  int start = !toggle * 180;
  int stop = toggle * 180;
  int incr = toggle? STEP : -STEP;

  for (int pos = start; pos != stop; pos += incr) {
    motor.write(pos);
    delay(DELAY);
  }

  motor.write(stop);
  delay(DELAY);
}

// --------------------------------------------- Schedule ------------------------------------------
void setNextEvent(const HatchEvent &event) { 
  if (event.openTime < nextEvent.openTime) {
    nextEvent = event;
  }
}

void addEvent(uint8_t hour, uint8_t minute, int openForSeconds) {
  HatchEvent event(hour, minute, openForSeconds);
  setNextEvent(event);
}

void addEvent(uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint32_t openForSeconds) {
  HatchEvent event(month, day, hour, minute, openForSeconds);
  setNextEvent(event);
}

void setAlarms() {
  TimeElements elements;
  breakTime(nextEvent.openTime, elements);

  clock.setAlarm2(elements.Day, elements.Hour, 
    elements.Minute, DS3231_MATCH_DT_H_M);

  breakTime(nextEvent.closeTime, elements);

  clock.setAlarm1(elements.Day, elements.Hour, 
    elements.Minute, elements.Second, DS3231_MATCH_DT_H_M_S);
}

void makeSchedule() {  
  // Update globals
  refreshDate();
  nextEvent = HatchEvent();

  // #1
  // 10:00, 11:00, 12:00, 13:00, 14:00  
  // brightening: 5:00 - 7:00, 19:00 - 21:00
  // + soliticies + equinoxes

  // Brightening
  addEvent(5, 00, 2 * SECS_PER_HOUR);
  addEvent(19, 00, 2 * SECS_PER_HOUR);

  // Analemma
  addEvent(10, 00, 2 * SECS_PER_MIN);
  addEvent(11, 00, 2 * SECS_PER_MIN);
  addEvent(12, 00, 2 * SECS_PER_MIN);
  addEvent(13, 00, 2 * SECS_PER_MIN);
  addEvent(14, 00, 2 * SECS_PER_MIN);
  
  // September equinox
  addEvent(9, 22, 4, 00, 19 * SECS_PER_HOUR); 

  // March equinox
  addEvent(3, 20, 4, 00, 19 * SECS_PER_HOUR); 

  // Summer solstice
  addEvent(6, 20, 4, 00, 19 * SECS_PER_HOUR); 

  // Winter solstice
  addEvent(12, 21, 4, 00, 19 * SECS_PER_HOUR); 

  setAlarms();
}

// #######################################################################################
void alarmFunction()
{
  isAlarm = true;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Blink to indicate it's release code
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
 
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(2, INPUT_PULLUP);
  
  clock.begin();

  // RTC is spamming with signals at startup
  clock.enable32kHz(false);
  clock.enableOutput(false);
    
  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  attachInterrupt(0, alarmFunction, FALLING);

  motor.attach(5);
  toggleMotor(Close);

  makeSchedule();

  isAlarm = false;
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {  
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  refreshDate();

  if (isAlarm) {
    isAlarm = false;
  
    if (clock.isAlarm1()) {
      clock.clearAlarm1();
      
      toggleMotor(Close);
  
      makeSchedule();
    }
    
    if (clock.isAlarm2()) {
      clock.clearAlarm2();
      
      toggleMotor(Open);
    }  
  }
}
