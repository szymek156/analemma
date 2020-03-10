
#include <LowPower.h>
#include <Time.h>
#include <TimeLib.h>

#include <Servo.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

#include <Wire.h>
#include <DS3231.h>


// TODO:
// Use hardware serial for GPS instead of software
// Use lib for finding timezone
// Add time adjusment from gps? meh probably
// TODO: dt.unixtime returns wrong value

// --------------   GPS    ------------------------
SoftwareSerial serialGPS = SoftwareSerial(3, 4);
TinyGPS gps;

// --------------   SERVO  ------------------------
Servo motor;
int position;

// ----------------- SCHEDULE  -------------------

enum EventType {
  EveryDay,
  EveryYear
};

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

    elements.Minute = minute;
    elements.Hour = hour;

    type = EveryDay;
  }

//  HatchEvent(uint8_t day, uint8_t hour, uint8_t minute, uint32_t openForSeconds) {
//    time_t currentTime = now();
//    TimeElements currentEl;
//    breakTime(currentTime, currentEl);
//
//    openTime = makeTime(TimeElements{
//      .Second = 0,
//      .Minute = minute,
//      .Hour = hour,
//      .Wday = dowInvalid, // Unused
//      .Day = day,
//      .Month = currentEl.Month,
//      .Year = currentEl.Year
//    });
//
//    // Event already passed
//    if (currentTime >= openTime) {
//      // Schedule for next month
//      // TODO: leap years?
//      openTime += SECS_PER_YEAR;
//    }
//
//    closeTime = openTime + openForSeconds;
//
//    elements.Minute = minute;
//    elements.Hour = hour;
//    elements.Day = day;
//    elements.Month = month;
//
//    type = EveryMonth;
//  }
  
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

    elements.Minute = minute;
    elements.Hour = hour;
    elements.Day = day;
    elements.Month = month;

    type = EveryYear;
  }
  
  time_t openTime;
  time_t closeTime;

  TimeElements elements;

  EventType type;

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

bool isGPSAvaliable() {
  int tries = 12;

  while (!serialGPS.available() && tries-- > 0) {
    delay(100);
  }

  bool avaliable = tries > 0;

  Serial.println("GPS Avaliable: " + String(avaliable));

  return avaliable;
}

void setSystemTimeFromGPS() {
  Serial.println("Waiting for GPS time ... ");
  bool time_set = false;

  while (!time_set) {
    while (serialGPS.available()) {
      char symbol = serialGPS.read();

      Serial.print(symbol);

      digitalWrite(LED_BUILTIN, HIGH);

      if (gps.encode(symbol)) { // process gps messages
        // when TinyGPS reports new data...
        unsigned long age;
        int y;
        byte mth, d, h, m, s;
        gps.crack_datetime(&y, &mth, &d, &h, &m, &s, NULL, &age);

        if (age < 500) {
          // set the Time to the latest GPS reading

          // TODO: adjust time
          clock.setDateTime(y, mth, d, h + offset, m, s);

          dt = clock.getDateTime();
          setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);

          time_set = true;

          Serial.println("System time set from GPS");
        }
      }
    }
    digitalWrite(LED_BUILTIN, LOW);
  }

  digitalWrite(LED_BUILTIN, HIGH);
}

void setSystemTimeFromPC() {
  clock.setDateTime(__DATE__, __TIME__);
  dt = clock.getDateTime();

  setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);

  Serial.println("System time set from PC");
}

bool rtcIsSet() {
  dt = clock.getDateTime();
  Serial.println(String("Current RTC time: ") + clock.dateFormat("d-m-Y H:i:s - l", dt));

  return dt.year > 2019;
}

void setTime() {
  if (rtcIsSet()) {
    dt = clock.getDateTime();
    setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);

    return;
  }

  Serial.println("Initialize GPS");
  serialGPS.begin(9600);

  if (isGPSAvaliable()) {
    setSystemTimeFromGPS();
  } else {
    setSystemTimeFromPC();
  }

  serialGPS.end();
}

void displaySystemClock() {
  static time_t prevDisplay = 0; // when the digital clock was displayed

  // Display every second
  if (now() != prevDisplay) {
    prevDisplay = now();

    char sz[32];
    sprintf(sz, "System: %02d/%02d/%02d %02d:%02d:%02d ",
            day(), month(), year(), hour(), minute(), second());
    Serial.println(sz);
  }
}

void displayRTCClock() {
  dt = clock.getDateTime();

  Serial.println(String("RTC: ") + clock.dateFormat("d-m-Y H:i:s - l", dt));
}

void displayUTCTime() {
  bool new_sentence = false;

  while (!new_sentence) {
    while (serialGPS.available()) {
      char symbol = serialGPS.read();

      if (gps.encode(symbol)) { // process gps messages
        // when TinyGPS reports new data...
        unsigned long age;
        int year;
        byte month, day, hour, minute, second;
        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &age);


        new_sentence = true;

        Serial.print("UTC:    ");

        if (age == TinyGPS::GPS_INVALID_AGE) {
          Serial.println("********** ******** ");
        } else
        {
          char sz[32];
          sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d, age %lu ",
                  month, day, year, hour, minute, second, age);
          Serial.println(sz);
        }
      }
    }
  }
}


// --------------------------------------------------- Servo ----------------------------------------
void toggleMotor2() {
  // Step and delay are set to motor opreate quietly
  static bool open = true;

  static const int STEP = 1;
  static const int DELAY = 40;

  int start = !open * 180;
  int stop = open * 180;
  int incr = open ? STEP : -STEP;

  for (int pos = start; pos != stop; pos += incr) {
    motor.write(pos);
    delay(DELAY);
  }

  motor.write(stop);

  open = !open;
}


// --------------------------------------------- Schedule ------------------------------------------
void setNextEvent(const HatchEvent &event) {
  Serial.println("Hatch open " + timet2str(event.openTime));
  Serial.println("Hatch close " + timet2str(event.closeTime));
  
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

void makeSchedule() {
  uint32_t start = millis();

  // Update globals
  dt = clock.getDateTime();
  nextEvent = HatchEvent();

  // Select closest event relative to current time
  addEvent(21, 37, 2 * SECS_PER_MIN);

  addEvent(3, 10, 6, 0, 2 * SECS_PER_HOUR); 



  Serial.println("Next open " + timet2str(nextEvent.openTime));

  Serial.println("Next close " + timet2str(nextEvent.closeTime));
  
  Serial.println("Schedule took[ms]: " + String(millis() - start));
}

// #######################################################################################


String timet2str(time_t point) {
  TimeElements el;
  breakTime(point, el);
  static char sz[32];

  sprintf(sz, "%4d/%02d/%02d %02d:%02d:%02d ",
          tmYearToCalendar(el.Year), el.Month, el.Day, el.Hour, el.Minute, el.Second);

  return sz;
}

void powerDownFor(unsigned int milliseconds) {
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  //   LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(9600);

  Serial.println("Analemma software! Setup components");

  Serial.println("Initialize Motor");
  motor.attach(9);
  motor.write(0);
  // Give some time to reach 0 position
  delay(1500);

  Serial.println("Initialize RTC");
  clock.begin();
  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  setTime();

  makeSchedule();

  Serial.println("Initialization complete");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  displayRTCClock();

  toggleMotor2();
  delay(1000);

  // Has to be last thing in the loop
  //  powerDownFor(30 * 1000);

  // Update system time from RTC
  dt = clock.getDateTime();
  setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
}
