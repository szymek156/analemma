/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Sweep
*/

#include <Servo.h>

Servo myservo;  /// create servo object to control a s/ervo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position
int lol = 5;
void setup() {
  myservo.attach(9);  // /attaches the servo on pin 9 to /the servo object
  TCCR1B &= ~((1 << CS11) | (1 << CS12));
  TCCR1B |= (1 << CS10);

//  pinMode(5, OUTPUT);
}

void loop() { 

//  for(int i=0; i<255; i++){
//    analogWrite(lol, i);
//    delay(50);
//  }
//
//  for(int i=254; i>0; i--){
//    analogWrite(lol, i);
//    delay(50);
//  }
//  

  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
//    analogWrite(5, pos);
    delay(5);                       // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
//    analogWrite(5, pos);
    delay(5);                       // waits 15ms for the servo to reach the position
  }
}
