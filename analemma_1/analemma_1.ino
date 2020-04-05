#include <LowPower.h>
#include <Time.h>
#include <TimeLib.h>

#include <Servo.h>
#include <SoftwareSerial.h>

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
// Play around with 
// https://www.nongnu.org/avr-libc/user-manual/group__avr__power.html

// --------------   GPS    ------------------------
SoftwareSerial serialGPS = SoftwareSerial(3, 4);

// --------------   SERVO  ------------------------
Servo motor;
enum Toggle {
  Close = 0,
  Open = 1
};

// -----------------  EEPROM   -------------------
int eeprom_addr = 0;
struct Record {
    time_t ts;
    time_t openTime;
    time_t closeTime;
    int id;
};
  
// ----------------- SCHEDULE  -------------------
int scheduleCounter = 0;

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
  
  Serial.println(String("RTC: ") + clock.dateFormat("d-m-Y H:i:s - l", dt));
}


// --------------------------------------------------- Servo ----------------------------------------
void toggleMotor(Toggle toggle) {  
  if (toggle == Open) {
    Serial.println("Open hatch " + String(scheduleCounter));
  } else {
    Serial.println("Close hatch " + String(scheduleCounter));
  }
  
  // Step and delay are set to motor opreate quietly
  static const int STEP = 1;
  static const int DELAY = 8;

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

void setAlarms() {
  TimeElements elements;
  breakTime(nextEvent.openTime, elements);

  clock.setAlarm2(elements.Day, elements.Hour, 
    elements.Minute, DS3231_MATCH_DT_H_M);

  breakTime(nextEvent.closeTime, elements);

  clock.setAlarm1(elements.Day, elements.Hour, 
    elements.Minute, elements.Second, DS3231_MATCH_DT_H_M_S);
}

void addEvents() {
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
}

void makeSchedule() {  
  // Update globals
  refreshDate();
  nextEvent = HatchEvent();

  addEvents();

  setAlarms();
  
  updateJournal();
  
  Serial.println("Next open  " + timet2str(nextEvent.openTime));
  Serial.println("Next close " + timet2str(nextEvent.closeTime));  
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

void alarmFunction()
{
  Serial.println("*** INT 0 ***");
  isAlarm = true;
}

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

void readJournal() {
  Serial.println("EEPROM Journal: ");
  int i = 0;
  Record record;
  
  while (i + sizeof(record) < EEPROM.length()) {  
    EEPROM_readAnything(i, record);
    i += sizeof(record);

    if (record.id == -1) {
      break;
    }
    
    Serial.println("#" + String(record.id) + " sys_time: " + timet2str(record.ts));
    Serial.println(" open: " + timet2str(record.openTime) + " close: " + timet2str(record.closeTime));
  }

  Serial.println("EEPROM Journal end");
}

void updateJournal() {
  Record record;
  
  if (eeprom_addr + sizeof(record) >= EEPROM.length()) {
    eeprom_addr = 0;
  }
  
  record.ts = now();
  record.openTime = nextEvent.openTime;
  record.closeTime = nextEvent.closeTime;
  record.id = scheduleCounter;
  
  EEPROM_writeAnything(eeprom_addr, record);
  eeprom_addr += sizeof(record);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Enable pullup resistor for RTC interrupt
  pinMode(2, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  // Divide clock by 8
  CLKPR = (1 << CLKPCE);
  CLKPR = (0 << CLKPS3) | (0 << CLKPS2) | (1 << CLKPS1) | (1 << CLKPS0);
  // From now on rates are divided by 8 and delays multiplied by 8
  
  // Wait a little to settle.
  delay(100);

  // In real it's 1200
  Serial.begin(9600);

  Serial.println("Analemma software! Setup components!");
 
  Serial.println("Initialize Motor...");
  motor.attach(5);
  // PWM prescaler, needs to be updated after clock division, and after attaching
  TCCR1B &= ~((1 << CS11) | (1 << CS12));
  TCCR1B |= (1 << CS10); 
    
  motor.write(0);
  delay(500);

  readJournal();
  
  Serial.println("Initialize RTC...");
  clock.begin();

  // RTC is spamming signals at startup
  clock.enable32kHz(false);
  clock.enableOutput(false);
    
  // Disarm and clear alarms because they are battery backed.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  attachInterrupt(0, alarmFunction, FALLING);

  refreshDate();

  makeSchedule();

  isAlarm = false;

  Serial.println("Initialization complete");
  Serial.flush();
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  Serial.println("Sleep...");
  Serial.flush();
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
