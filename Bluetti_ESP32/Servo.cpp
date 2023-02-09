#include <ESP32Servo.h>
#include "BTooth.h"

#define SERVO_PIN 13 // ESP32 pin GIOP13 connected to servo motor

Servo servoMotor;

void initServo(){
  servoMotor.attach(SERVO_PIN);  // attaches the servo on ESP32 pin
  servoMotor.write(0); // on start go to pos 0
 }

 void triggerButtonPress(int degree) {
  // rotates from 0 degrees to degree
  for (int pos = 0; pos <= degree; pos += 1) {
    // in steps of 1 degree
    servoMotor.write(pos);
    delay(15); // waits 15ms to reach the position
  }

  // rotates from degree back to 0 degrees
  for (int pos = degree; pos >= 0; pos -= 1) {
    servoMotor.write(pos);
    delay(15); // waits 15ms to reach the position
  }
  delay(1000);
  //FIXME: find right method to do a rescan, initBluetooth() is hard crashing device (but working ;-))
  initBluetooth();
}
