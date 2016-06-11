/*
 * uc0.ino
 * 
 * Rover firmware code for arduino uno
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include <stdint.h>
#include <Wire.h>

#define SIDE_LEFT_FWD_DRIVE 10
#define SIDE_LEFT_REAR_DRIVE 11
#define SIDE_RIGHT_FWD_DRIVE 5
#define SIDE_RIGHT_REAR_DRIVE 6
#define SPEED_MAX 255
#define SPEED_MIN -255
#define US_ECHO_TRIG_PIN 9
#define US_ECHO_ECHO_PIN 8
#define SND_SPEED_USEC_CM 29.387
#define DEV_ADDR 0x05

/*
 * Uncomment for serial debug output
 */
//THERE IS A BUG CAUSING THE I2C TO NOT WORK PROPERLY
//WHEN SERIAL OUTPUT IS ENABLED
//#define DEBUG 

/* 
Speed of sound = 340.29 m/s = 34029 cm/s
Time per distance 1/34029 s/cm to usec [*1*10e6] = 29.387 usec/cm
*/

enum Wheel {WheelLeft, WheelRight}; 

//Command [command:int16_t, param:int16_t]
//Return data [command:int16_t, data:int16_t] 
enum command {
    NOP = 0x0,
    REQ_LEFT_SPEED = 0x01,
    REQ_RIGHT_SPEED = 0x02,
    SET_LEFT_SPEED = 0x03,
    SET_RIGHT_SPEED = 0x04,
    SET_TOTAL_SPEED = 0x05,
    WHEELS_STOP = 0x06,
    REQ_DISTANCE = 0x07,
    INIT_CONTROL_STATE = 0xF0,
    ERR = 0xFF
}; 


int currentSpeedLeft = 0;
int currentSpeedRight = 0;
uint16_t currentDistance = 99;
command lastCommand = NOP; 
int16_t lastCommandParam = 0;
int16_t errCode =0; //0-no error

/*
 * (2pins per side setting h-bridge to turn motors front ot back)
 * ONLY ONE PIN ACTIVE PER SIDE OTHERWISE WE WILL HAVE SHORT AT H-BRIDGE !!!!!!!!!
 * setSpeedForSide andsetSpeed functions have to be defined with priority on clearly coverage of all use cases!!
 */
void setSpeedForSide(int requestedSpeed, int* currentSpeed, unsigned forwardDrivePin, unsigned rearDrivePin)
{


  if(requestedSpeed == 0) {
    analogWrite(forwardDrivePin, 0);
    analogWrite(rearDrivePin, 0);
  }
  else if(*currentSpeed > 0 && requestedSpeed < 0) {
    analogWrite(forwardDrivePin, 0);
    analogWrite(rearDrivePin, -requestedSpeed);
  }
  else if(*currentSpeed < 0 && requestedSpeed > 0) {
    analogWrite(rearDrivePin, 0);
    analogWrite(forwardDrivePin, requestedSpeed);
  }
  else if(*currentSpeed > 0 && requestedSpeed > 0) {
    analogWrite(forwardDrivePin, requestedSpeed);
  }
  else if(*currentSpeed < 0 && requestedSpeed < 0) {
    analogWrite(rearDrivePin, -requestedSpeed);
  }
  
  *currentSpeed = requestedSpeed;
}

void setSpeed(int requestedSpeed, Wheel wheel)
{
  if(requestedSpeed < SPEED_MIN || requestedSpeed > SPEED_MAX) {
#ifdef DEBUG
        Serial.print("Requested speed out of range !!! "); Serial.println(requestedSpeed);
#endif
   return; 
  }
  
  if(wheel == WheelLeft) {
    setSpeedForSide(requestedSpeed, &currentSpeedLeft, SIDE_LEFT_FWD_DRIVE, SIDE_LEFT_REAR_DRIVE);
  }
  else if(wheel == WheelRight) {
    setSpeedForSide(requestedSpeed, &currentSpeedRight, SIDE_RIGHT_FWD_DRIVE, SIDE_RIGHT_REAR_DRIVE);
  }
}

void calculateDistance()
{
  digitalWrite(US_ECHO_TRIG_PIN, LOW);
  delayMicroseconds(20);
  digitalWrite(US_ECHO_TRIG_PIN, HIGH);
  delayMicroseconds(100);
  digitalWrite(US_ECHO_TRIG_PIN, LOW);
  
  int pulseUSecs = pulseIn(US_ECHO_ECHO_PIN, HIGH);
  
  currentDistance = (pulseUSecs <=0) ? 100/*one meter*/ : (pulseUSecs / SND_SPEED_USEC_CM / 2.0f);
}

inline void toBuffer(uint8_t *buff4, int16_t cmd, int16_t dta)
{
    int16_t *tmp = (int16_t*)buff4;
    tmp[0] = cmd;
    tmp[1] = dta;
}


//callback for host requesting data
void callbackReqDta()
{
    uint8_t buff[4];
  
#ifdef DEBUG
    Serial.println(lastCommand);
#endif
  
    switch(lastCommand)
    {
    case NOP:
        toBuffer(buff, NOP, lastCommandParam);
        break;
    case REQ_LEFT_SPEED:
        toBuffer(buff, REQ_LEFT_SPEED, currentSpeedLeft);
        break;
    case REQ_RIGHT_SPEED:
        toBuffer(buff, REQ_RIGHT_SPEED, currentSpeedRight);
        break;
    case SET_LEFT_SPEED:
        toBuffer(buff, SET_LEFT_SPEED, lastCommandParam);
        break;
    case SET_RIGHT_SPEED:
        toBuffer(buff, SET_RIGHT_SPEED, lastCommandParam);
        break;
    case SET_TOTAL_SPEED:
        toBuffer(buff, SET_TOTAL_SPEED, lastCommandParam);
        break;
    case WHEELS_STOP:
        toBuffer(buff, WHEELS_STOP, currentSpeedLeft | currentSpeedRight);
        break;
    case REQ_DISTANCE:
        toBuffer(buff, REQ_DISTANCE, currentDistance);
        break;
    case INIT_CONTROL_STATE:
        toBuffer(buff,INIT_CONTROL_STATE, 0x5AAA);
        break;
    case ERR:
        toBuffer(buff, ERR, errCode);
        break;
    default:    
#ifdef DEBUG
        Serial.print("Undefined value !!! "); Serial.println(lastCommand);
#endif
    break;
    }

    Wire.write(buff, 4);
}

//callback for host sending a command
void callbackRecvCommand(int bytecount)
{
  
    if(bytecount != 4) {
#ifdef DEBUG
        Serial.print("Wrong command size[bytes]: "); Serial.println(bytecount);
#endif
    }
    else {
        int16_t cmd = Wire.read();
        cmd |=   (Wire.read() << 8);
        lastCommand = (command)cmd;
        lastCommandParam = 0;
        lastCommandParam = Wire.read();
        lastCommandParam |= (Wire.read() << 8);


#ifdef DEBUG
        Serial.println(Wire.available());
        Serial.print("Command: ");
        Serial.print(lastCommand);
        Serial.print(" param: ");
        Serial.println(lastCommandParam);
#endif
        switch(lastCommand)
        {
        case SET_LEFT_SPEED:
            setSpeed(lastCommandParam, WheelLeft);
            break;
        case SET_RIGHT_SPEED:
            setSpeed(lastCommandParam, WheelRight);
            break;
        case SET_TOTAL_SPEED:
            setSpeed(lastCommandParam, WheelLeft);
            setSpeed(lastCommandParam, WheelRight);
            break;
        case WHEELS_STOP:
            setSpeed(0, WheelLeft);
            setSpeed(0, WheelRight);
            break;
        case INIT_CONTROL_STATE:
            setSpeed(0, WheelLeft);
            setSpeed(0, WheelRight);
            break;    
        default:
            break;
        }
    }
}

void setup()
{
    pinMode(US_ECHO_TRIG_PIN, OUTPUT);
    pinMode(US_ECHO_ECHO_PIN, INPUT);

    lastCommand = INIT_CONTROL_STATE;

    Wire.begin(DEV_ADDR);
    Wire.onReceive(callbackRecvCommand);
    Wire.onRequest(callbackReqDta);

#ifdef DEBUG
    Serial.begin(9600);
#endif
}

#define DISTANCE_READ_INTERVAL 500UL

void loop()
{
    unsigned long currentTime = millis();
    while(true) {
        if((millis() - currentTime) > DISTANCE_READ_INTERVAL) {
            calculateDistance();
            currentTime = millis();
        }
    }
}
