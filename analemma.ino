
#include <LowPower.h>

#include <Time.h>
#include <TimeLib.h>

#include <Servo.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

// TODO:
// Use hardware serial for GPS instead of software
// Use lib for finding timezone
// Use RTC clock
// Add time adjusment from gps? meh probably


// --------------   GPS    ------------------------
SoftwareSerial serialGPS = SoftwareSerial(2, 3);
TinyGPS gps; 

// --------------   SERVO  ------------------------
Servo motor;
int position;

// --------------   TIME   ------------------------
const int offset = 1;   // Central European Time
//const int offset = -5;  // Eastern Standard Time (USA)
//const int offset = -4;  // Eastern Daylight Time (USA)
//const int offset = -8;  // Pacific Standard Time (USA)
//const int offset = -7;  // Pacific Daylight Time (USA)
//time_t prevDisplay = 0; // when the digital clock was displayed


void setSystemTimeFromGPS() {
  Serial.println("Waiting for GPS time ... ");

  bool led_status = true;  
  bool time_set = false;

  digitalWrite(LED_BUILTIN, led_status);
  
  while (!time_set) {  
    while (serialGPS.available()) {
      char symbol = serialGPS.read();
      
      Serial.print(symbol);
      
      if (gps.encode(symbol)) { // process gps messages        
        led_status = !led_status;
        digitalWrite(LED_BUILTIN, led_status);

        
        // when TinyGPS reports new data...
        unsigned long age;
        int year;
        byte month, day, hour, minute, second;
        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &age);
        
        if (age < 500) {
          // set the Time to the latest GPS reading
          setTime(hour, minute, second, day, month, year);
          adjustTime(offset * SECS_PER_HOUR);

          time_set = true;
          
          Serial.println("System time set");
        }
      }
    }
  }

  digitalWrite(LED_BUILTIN, LOW);
}

void adjustSystemTime(uint32_t start) {
  static const uint32_t MILLIS_CONV = 1000;
  
  // WDG SUCKS at time measure, so add some margin drift, it's not perfect but absolutely enough to our needs
  static const uint32_t WDG_DRIFT_MS = 750;
  static uint32_t execution = 0;
  
  execution += millis() - start + WDG_DRIFT_MS;
 
  uint32_t adjust = 8 * MILLIS_CONV + execution;
  
  adjustTime(adjust/MILLIS_CONV);

  execution = adjust % MILLIS_CONV;
}

void displaySystemClock(){
  static time_t prevDisplay = 0; // when the digital clock was displayed

  // Display every second
  if (now() != prevDisplay) {
    prevDisplay = now();
    
    char sz[32];
    sprintf(sz, "System: %02d/%02d/%02d %02d:%02d:%02d ",
        month(), day(), year(), hour(), minute(), second());
    Serial.println(sz);  
  }  
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
        
        if (age == TinyGPS::GPS_INVALID_AGE){
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


void powerDownFor(unsigned int milliseconds) {
//   LowPower.powerSave(SLEEP_8S, ADC_OFF, BOD_OFF, TIMER2_ON);
//   LowPower.powerStandby(SLEEP_8S, ADC_OFF, BOD_OFF);  
   
   LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
//   LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    
//  LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
//                SPI_OFF, USART0_OFF, TWI_OFF);

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

  open = !open;
}

void setup() {  
  Serial.begin(9600);

  Serial.println("Analemma software! Setup components");
  
  motor.write(0);
  
  serialGPS.begin(9600);
  motor.attach(9);
  
//  setSystemTimeFromGPS();

  serialGPS.end();
  
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  displaySystemClock();

  toggleMotor2();
  delay(1000);
  
  // Has to be last thing in the loop
//  powerDownFor(30 * 1000);

}
