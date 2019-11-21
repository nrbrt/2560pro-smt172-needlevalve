// needlevalve control, 2 smt172 temperature sensors and a MPXV7002DP differential pressure sensor (5V)
//by nrbrt
//GPLv3 applies

#include <arduino.h>
#include <AccelStepper.h>
#include <SMT172_T4.h>
#include <SMT172_T5.h>

//Define stepper motor connections
#define dirPin 12
#define stepPin 11

// prepare microstepping modes
#define m0Pin 29
#define m1Pin 27
#define m2Pin 25

int fullStep[4] = {0,0,0};
int halfStep[4] = {1,0,0};
int quarterStep[4] = {0,1,0};
int eighthStep[4] = {1,1,0};
int sixteenthStep[4] = {0,0,1};
int thirtysecondthStep[4] = {1,1,1};

// enable/disable motor
#define enablePin 31

// select the input pin for the Pressure Sensor
int sensorPin = A0;    

#define home_switch 10 // Pin 10 connected to Home Switch (MicroSwitch (NO))

//Create stepper object
AccelStepper stepper(1,stepPin,dirPin); //motor interface type must be set to 1 when using a driver.

long last_update = 0;
long time_now;
float temp1 = 0;
float temp2 = 0;
String str; //string received by serial port
int count; //amount of elements after splitting the received string
String sParams[10];
bool calibration_done = false; //no movement before calibration
bool moving = false; //used to determine if the motor is moving
long initial_homing=-1;  // Used to Home Stepper at startup
float motorspeed = 300;
float motoraccel = 3000;
float voltage=0;
float pressure_kPa=0;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  SMT172_T4::startTemperature(0.001);
  SMT172_T5::startTemperature(0.001);
  
  //default enable motor and set full step mode
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin,LOW);
  pinMode(m0Pin, OUTPUT);
  digitalWrite(m0Pin, LOW);
  pinMode(m1Pin, OUTPUT);
  digitalWrite(m1Pin, LOW);
  pinMode(m2Pin, OUTPUT);
  digitalWrite(m2Pin, LOW);
  
  pinMode(home_switch, INPUT_PULLUP); //enable internal pullup
  pinMode(sensorPin, INPUT);  // Pressure sensor is on Analog pin 0
  delay(30);
}


void loop() {
  time_now = millis();
  
  //check if the motor is running to a position or not
  if(stepper.distanceToGo() != 0){
     moving = true;
   }else{
     moving = false;
   }

  //if 1 second has passed, since last measurement and the motor is not moving, read temperature
  //again and send it 
  if((time_now - last_update) >= 1000){
        getTemp();
        voltage = analogRead(sensorPin)*(5.0 / 1023.0);
        pressure_kPa = voltage - 2.5;
        Serial.print("t1:");
        Serial.print(temp1);
        Serial.print(",t2:");
        Serial.print(temp2);
        Serial.print(",p:");
        Serial.println(pressure_kPa);
        temp1 = 0;
        temp2 = 0;
        last_update = millis();
  }

  if(Serial.available()){
     str = Serial.readStringUntil('\n');
     count = StringSplit(str,':',sParams,6);
       
     
     //check homeswitch. Use=> es Returns 0(triggered) or 1(not triggered)
     if(sParams[0] == "es" && count == 1){
        CheckHomeswitch();
     }
       
     
     //calibrate zero position needlevalve. Use=> cal:steps_beyond_homeswitch:motor_acceleration:motor_speed Returns nothing
     if(sParams[0] == "cal" && count == 4){
        CalibrateNeedleValve(sParams[1].toInt(),sParams[2].toFloat(),sParams[3].toFloat());
     }
     
     
     //move non-blocking to absolute position. Use=> move:target_position:motor_acceleration:motor_speed Returns nothing unless not calibrated
     if(sParams[0] == "move" && count == 2){
        MoveTo(sParams[1].toInt());
     }
     
     
     //move blocking to absolute position. Use=> pos:target_position:motor_acceleration:motor_speed Returns nothing unless not calibrated
     if(sParams[0] == "pos" && count == 2){
        MoveToPosition(sParams[1].toInt());
     }
     
     // define speed settings
     if(sParams[0] == "spd" && count == 2){
      motorspeed = sParams[1].toFloat();
      stepper.setMaxSpeed(motorspeed);
      stepper.setSpeed(motorspeed);
     }
     
     // define acceleration settings
     if(sParams[0] == "acc" && count == 2){
      motoraccel = sParams[1].toFloat();
      stepper.setAcceleration(motoraccel);
     }

     // enable or disable the motor
     if(sParams[0] == "motor" && count == 2){
      if(sParams[1] == "enable"){
        digitalWrite(enablePin, LOW);
      }else{
        digitalWrite(enablePin, HIGH);
      }
     }

     // selection of microstepping mode
     if(sParams[0] == "mode" && count == 2){
      if(sParams[1] == "1"){
        digitalWrite(m0Pin,fullStep[0]);
        digitalWrite(m1Pin,fullStep[1]);
        digitalWrite(m2Pin,fullStep[2]);
      }
      if(sParams[1] == "2"){
        digitalWrite(m0Pin,halfStep[0]);
        digitalWrite(m1Pin,halfStep[1]);
        digitalWrite(m2Pin,halfStep[2]);
      }
      if(sParams[1] == "4"){
        digitalWrite(m0Pin,quarterStep[0]);
        digitalWrite(m1Pin,quarterStep[1]);
        digitalWrite(m2Pin,quarterStep[2]);
      }
      if(sParams[1] == "8"){
        digitalWrite(m0Pin,eighthStep[0]);
        digitalWrite(m1Pin,eighthStep[1]);
        digitalWrite(m2Pin,eighthStep[2]);
      }
      if(sParams[1] == "16"){
        digitalWrite(m0Pin,sixteenthStep[0]);
        digitalWrite(m1Pin,sixteenthStep[1]);
        digitalWrite(m2Pin,sixteenthStep[2]);
      }
      if(sParams[1] == "32"){
        digitalWrite(m0Pin,thirtysecondthStep[0]);
        digitalWrite(m1Pin,thirtysecondthStep[1]);
        digitalWrite(m2Pin,thirtysecondthStep[2]);
      }
     }
  }
  stepper.run();
}

//find sensor and get temperature
void getTemp(){
  SMT172_T4::startTemperature(0.001);
  repeat_T4:
    switch (SMT172_T4::getStatus()) {
    case 0: goto repeat_T4; // O Dijkstra, be merciful onto me, for I have sinned against you :)
    case 1: 
      temp1 = SMT172_T4::getTemperature();
      break;
    case 2:
      temp1 = -1;
  }
  
  stepper.run();
  
  SMT172_T5::startTemperature(0.001);
  repeat_T5:
    switch (SMT172_T5::getStatus()) {
    case 0: goto repeat_T5; // O Dijkstra, be merciful onto me, for I have sinned against you :)
    case 1:
      temp2 = SMT172_T5::getTemperature();
      break;
    case 2:
      temp2 = -1;
  }
}


//split string
int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams)
{
    int iParamCount = 0;
    int iPosDelim, iPosStart = 0;
    
    do {
        // Searching the delimiter using indexOf()
        iPosDelim = sInput.indexOf(cDelim,iPosStart);
        if (iPosDelim > (iPosStart+1)) {
            // Adding a new parameter using substring() 
            sParams[iParamCount] = sInput.substring(iPosStart,iPosDelim);
            iParamCount++;
            // Checking the number of parameters
            if (iParamCount >= iMaxParams) {
                return (iParamCount);
            }
            iPosStart = iPosDelim + 1;
        }
    } while (iPosDelim >= 0);
    if (iParamCount < iMaxParams) {
        // Adding the last parameter as the end of the line
        sParams[iParamCount] = sInput.substring(iPosStart);
        iParamCount++;
    }
    return (iParamCount);
}


//non-blocking move to absolute postion
void MoveTo(int steps){
  //only move if calibrated
  if(calibration_done){
    
    //if we are not moving, move immediately, otherwise stop first, then move
    //if(moving){
    //  stepper.stop();
    //}
    stepper.moveTo(steps);
    stepper.run();
    
  }else{
    Serial.println("not calibrated");
  }
  
}


//blocking move to absolute position
void MoveToPosition(int absolutepos){
  //only move if calibrated
  if(calibration_done){
    //if the motor is not moving, move immediately, otherwise stop first, then move
    if(!moving){
      stepper.runToNewPosition(absolutepos);
    }else{
      stepper.stop();
      stepper.runToNewPosition(absolutepos);
    }
  }else{
    Serial.println("not calibrated");
  }
}


//check current state homing switch. Normally Open microswitch between gnd and the input pin with internal pull-up enabled, so 1 is not triggered, 0 is triggered
void CheckHomeswitch(){
  Serial.print("es:");
  Serial.println(digitalRead(home_switch));
}


//calibrate zero position, using the homing switch as a fixed point from where to count steps back to the closed
//position of the needle valve and set zero
void CalibrateNeedleValve(int steps_past_home, float calibrationaccel, float calibrationspeed){
   //if the motor is moving for some reason, stop first
   if(moving){
    stepper.stop();
   }
   //set the speed to use during the calibration
   stepper.setMaxSpeed(calibrationspeed);
   stepper.setAcceleration(calibrationaccel);
  
   //homing switch not initially triggered NO (normally open)
   while (digitalRead(home_switch)) {  // Make the Stepper move CW until the switch is activated   
    stepper.moveTo(initial_homing);  // Set the position to move to
    initial_homing++;  // Increase by 1 for next move if needed
    stepper.run();  // Start moving the stepper
    delay(5);
   }

   stepper.setCurrentPosition(0);  // Set the current position as zero for now
   stepper.setMaxSpeed(calibrationspeed);      // Set Max Speed of Stepper (Slower to get better accuracy)
   stepper.setAcceleration(calibrationaccel);  // Set Acceleration of Stepper
   initial_homing=1;

   //homing switch initially triggered NO (normally open)
   while (!digitalRead(home_switch)) { // Make the Stepper move CCW until the switch is deactivated
     stepper.moveTo(initial_homing);  
     stepper.run();    // Start moving the stepper
     initial_homing--; //Decrease by 1 for next move if needed
     delay(5);
   }

   //temporary zero position
   stepper.setCurrentPosition(0); 
   stepper.moveTo(steps_past_home);

   //run motor until the position past the homing switch is reached
   while(stepper.distanceToGo() != 0){
     stepper.run();
   }
  
   //final zero position, calibration done
   stepper.setCurrentPosition(0);
   //set the speed and acceleration for normal operation
   stepper.setMaxSpeed(motorspeed);
   stepper.setSpeed(motorspeed);
   stepper.setAcceleration(motoraccel);
   calibration_done = true;
}
