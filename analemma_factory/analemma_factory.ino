
#include <Time.h>
#include <TimeLib.h>

#include <TinyGPS.h>
#include <SoftwareSerial.h>

#include <Wire.h>
#include <DS3231.h>
#include <EEPROM.h>


// --------------   GPS    ------------------------
SoftwareSerial serialGPS = SoftwareSerial(3, 4);
TinyGPS gps;

// -----------------  EEPROM   -------------------
int eeprom_addr = 0;
struct Record {
    time_t ts;
    time_t openTime;
    time_t closeTime;
    int id;
};

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
          //TODO: +offset overflows: 25:03
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


void setTime() {
  Serial.println("Initialize GPS");
  serialGPS.begin(9600);

  if (isGPSAvaliable()) {
    setSystemTimeFromGPS();
  } 

  serialGPS.end();
}

void refreshDate() {
  dt = clock.getDateTime();
  setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
  
  Serial.println(String("RTC: ") + clock.dateFormat("d-m-Y H:i:s - l", dt));
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

void clearJournal() {
  Serial.println("CLEARING JOURNAL, type yes to continue");

  while(!Serial.available()){}

  String answer = Serial.readString();

  if (answer == "yes\n") {
    Serial.println("CLEARING...");
    
    for(int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, -1);
    }  
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(9600);

  Serial.println("Analemma software! Factory Reset!");

  Serial.println("EEPROM length " + String(EEPROM.length()));
  readJournal();

  clearJournal();
  
  Serial.println("Initialize RTC...");
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
  
  setTime();

  Serial.println("Factory reset complete");
  Serial.flush();
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  refreshDate();
  delay(1000);
}
