#include <Servo.h>

#define PIN_SERVO 10

#define _DUTY_MIN 500
#define _DUTY_MAX 2500
#define _SERVO_SPEED 3.0
#define MOVE_TIME 60000

Servo myservo;

unsigned long startTime;
float currentDuty = _DUTY_MIN;

void setup(){
  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_DUTY_MIN);
  startTime = millis();
  Serial.begin(52800);
}

void loop(){
  unsigned long elapsed = millis() - startTime;

  if(elapsed <= MOVE_TIME){
    float progress = (float)elapsed / MOVE_TIME;

    currentDuty = _DUTY_MIN + progress * (_DUTY_MAX - _DUTY_MIN);

    myservo.writeMicroseconds(currentDuty);
  }
  else{
    myservo.writeMicroseconds(_DUTY_MAX);
  }

  delay(20);
}