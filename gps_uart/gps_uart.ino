#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <RTC.h>

SoftwareSerial gpsSerial(2, 3); // RX, TX
TinyGPS gps;
String sentence;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);

  Serial.print("Serial initialization");
}

void PrintFix() {
  float flat, flon;
  unsigned long fix_age; // returns +- latitude/longitude in degrees
  gps.f_get_position(&flat, &flon, &fix_age);
  if (fix_age == TinyGPS::GPS_INVALID_AGE)
    Serial.println("No fix detected");
  else if (fix_age > 5000)
    Serial.println("Warning: possible stale data!");
  else
    Serial.println("Data is current: " + String(fix_age));
  
}
void PrintStats() {
  unsigned long chars;
  unsigned short sentences, failed_checksum;
  gps.stats(&chars, &sentences, &failed_checksum);
  Serial.println(String("Stats: sentences ") + sentences + " failed checksum " + failed_checksum);
}

void PrintSentence(char new_symbol) {    
    sentence += new_symbol;
    
    if (new_symbol == '\n') {
      Serial.print(sentence);
      sentence = "";

      PrintFix();
    }
}

void loop() {
  
  
  while (gpsSerial.available() > 0) {
    char new_symbol = gpsSerial.read();

    PrintSentence(new_symbol);

    if (gps.encode(new_symbol)) {
      long lat, lon;
      unsigned long fix_age, time, date, speed, course;
      unsigned long chars;
      unsigned short sentences, failed_checksum;
       
      // retrieves +/- lat/long in 100000ths of a degree
      gps.get_position(&lat, &lon, &fix_age);
       
      // time in hhmmsscc, date in ddmmyy
      gps.get_datetime(&date, &time, &fix_age);
       
      // returns speed in 100ths of a knot
      speed = gps.speed();
       
      // course in 100ths of a degree
      course = gps.course();
    }
    
    
    
  }
}
