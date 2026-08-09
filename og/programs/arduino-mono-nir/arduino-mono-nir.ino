// Stepper motor control for NIR monochromator, including manual positioning and 
// automated scanning.
// This sketch builds on a "basic lab instrument" design with standardized LCD and
// pushbutton interfaces. The physical layout additionally includes a custom motor
// shield incorporating a DRV8825 bipolar stepper motor driver and limit switch connection,
// and a manual toggle switch to change between manual mode (switch closed) and 
// auto mode (switch open). 
// It should additionally be possible to trigger automatic scans via an external, active-low
// signal to startnextButtonPin, and to monitor automatic scan starts via a rising-edge TTL 
// signal on the ledPin.
// Timing is via delayMicroseconds() calls. Future developments could enable more precise
// or flexible control via elapsed time. 
// The range of accessible speeds can be adjusted via  microstepping settings for the DRV8825,
// and different monochromators could be accommodated via wavelength calibrations.
// The code enables replicate scans in auto mode, but the user interface step to specify the
// number of scans has not yet been added.
// Homing on startup is currently optional, but is strongly advised for actual use because 
// position limits are not monitored in hardware after the initial homing step.
// Made by Abdulla Shaker and Andrew Greytak. Update this repo for latest information.

// Attribution: Greytak Chemistry Laboratory - University of South Carolina.
// Copyright Andrew B Greytak 2025 with applicable rights reserved by USC:
// github.com/greytak-chemistry-lab/arduino-bli-mono-controller
//
// See arduino-bli-mono-controller/LICENSE file for license info
// variation/version = og  (current/development version)

#include <LiquidCrystal.h>

// Define pin numbers for LCD display
const int lcdRSPin = 2;
const int lcdEPin = 3;
const int lcdD4Pin = 4;
const int lcdD5Pin = 5;
const int lcdD6Pin = 6;
const int lcdD7Pin = 7;

// Define input pin numbers
const int upButtonPin = 14;
const int downButtonPin = 15;
const int startnextButtonPin = 16;
const int stopresetButtonPin = 17;
const int limit1Pin = 18;
const int autoManualPin = 19;  // LOW (switch closed) = auto

const int HOLDOFF = 150;     // milliseconds after button press before we read again
const int INITDELAY = 2000;  // delay on setup()
const int STARTDELAY = 500;  // delay after next before accepting start signal
const int CYCLEDELAY = 50;   // for polling button presses and limit switches while moving

// Define LED pin
const int ledPin = LED_BUILTIN;

// Motor control and microstepping pins
const int dirPin = 8;
const int stepPin = 9;
//  changed microstepping pins to 10,11,12 so 13 is available for LED
const int M0 = 10;
const int M1 = 11;
const int M2 = 12;

// Motor settings
const long stepsPerRevolution = 1600;  // 1/8 microstepping
// Each step takes at least 2*stepInterval microseconds
// stepInterval in us: 1000000 / (2*nm_per_min*8/60) with c1=(-)8
unsigned long stepInterval = 6250;  // 6250 microseconds for 80 steps per second
const long backlashSteps = 10;
const long steps_Min = backlashSteps;
const long steps_Max = 8 * stepsPerRevolution;  // SET APPROPRIATELY TO AVOID CRASH

// Direction definitions
// With our setup, CLOCKWISE moves FORWARD (away from limit1 switch)
// COUNTERCLOCKWISE moves BACKWARD (toward limit1 switch)
// Backward moves should be followed by backlash correction
#define CLOCKWISE 1
#define COUNTERCLOCKWISE 0

// Polynomial constants to convert nm position to steps, from calibration
float c0 = 0;
// change c1 to -8 since increasing steps leads to shorter wavelength position
float c1 = -8;  // AS: 8 steps per nm. 80 steps per second. So 10 nm/s scanning
float c2 = 0;
float c3 = 0;

// Wavelength configuration and counters
const long steps_offset = 1328;   // Offset after homing
long stepCounter = 0;            // Tracks current step position from home
const long startPosition = 1990;  // Start position in nm
long position;                   // current position in nm
const long position_Max = 2000;
const long position_Min = 700;
const int position_incr = 10; // increment for position selection button presses 
long autoscan_Start,autoscan_End,autoscan_Speed,autoscan_Cycles; // need these to be global so they don't get lost switching auto/manual
bool isFirstStart = true;  // Flag to indicate we are on very first start
bool hardReset = true; // Flag to indicate that scan parameters should be set to default on setup() or next start of loop
bool isHomed = false;      // flag to indicate homing is not done yet


// speed selection
long speed_nm_min=0; // selected speed in nm/min
long slow_us=0;   // delay based on speed
const long speed_Min=300; // minimum speed, nm/min: slower than 300 incompatible with current delay scheme
const long speed_Max=600; // Speed max, nm/min, max=600 for stepInterval=6250
const long speed_incr=60; // multiple of 60
long autoscan_speed=0; //initialize autoscan speed 

// Initialize LCD
LiquidCrystal lcd(lcdRSPin, lcdEPin, lcdD4Pin, lcdD5Pin, lcdD6Pin, lcdD7Pin);


// Function to calculate motor steps based on target wavelength
long positionSteps(long position_requested) {
  double prf, stepsf;
  prf = (double)(position_requested - startPosition);
  stepsf = c0 + c1 * prf + c2 * pow(prf, 2) + c3 * pow(prf, 3);
  return (long)(stepsf) + steps_offset;  // Return as long integer since motor steps are discrete
}



// Function to calculate current wavelength from step count, currently assume linear 
long stepsPosition(long currentSteps) {
  double stepsf, pf;
  stepsf = (double)(currentSteps - steps_offset);
  pf = (stepsf - c0) / c1;
  return (long)(pf) + startPosition;
}

// Function to round position_target
long positionRound(long position_requested) {
  return position_requested + ( (position_incr-1) - ((position_requested-1)%position_incr) );
}

// Function to display current position
void displayPosition() {
    lcd.setCursor(0, 1);
    position=stepsPosition(stepCounter);
    lcd.print(position);
    lcd.write("nm");
    lcd.setCursor(8, 1);
    lcd.print(stepCounter);
    lcd.write("st");
}

// Function to handle reset button presses
int resetHandler(int errorPin) {
  int resetval=1;
  long resetTime, elapsedTime;
  resetTime=millis();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Fault! Pin ");
  lcd.print(errorPin);
  position=positionSteps(stepCounter);
  displayPosition();
  while(digitalRead(errorPin) == LOW) { // Button is still pressed
    delay(HOLDOFF);
  }
  delay(STARTDELAY);
  lcd.setCursor(0,0);
  lcd.print("Fault:Next/Reset");
  while(digitalRead(startnextButtonPin)==HIGH && digitalRead(stopresetButtonPin)==HIGH) {
    delay(HOLDOFF);
    if(digitalRead(startnextButtonPin)==LOW) { // clear error and continue
      resetval=0;
      break;
    }
    if(digitalRead(stopresetButtonPin)==LOW) {
      resetTime=millis();
      while(digitalRead(stopresetButtonPin)==LOW) {
        elapsedTime=millis() - resetTime;
        if(elapsedTime > INITDELAY) {
          resetval=2; // hard reset
          break;
        }
      }
      break;
    }
  }
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Resetval:");
lcd.print(resetval);
delay(STARTDELAY);
return resetval;
}

// Function to apply backlash correction
void applyBacklashCorrection() {
  int i;
  lcd.setCursor(0, 1);
  lcd.print("Backlash Correct");  // Feedback to show backlash correction
  digitalWrite(dirPin, COUNTERCLOCKWISE);
  for (i = 0; i < backlashSteps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepInterval);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepInterval);
    stepCounter--;
  }
  delay(HOLDOFF);
  digitalWrite(dirPin, CLOCKWISE);
  for (i = 0; i < backlashSteps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepInterval);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepInterval);
    stepCounter++;
  }
  lcd.setCursor(0, 1);            // Clear backlash message
  lcd.print("                ");  // Clear the line
}


// Function to perform homing and set steps_offset
void homeMotor() {
  // Move reverse until limit1 switch is closed
  digitalWrite(dirPin, COUNTERCLOCKWISE);                                              // Set motor direction to backward
  while (digitalRead(limit1Pin) == HIGH && digitalRead(stopresetButtonPin) == HIGH) {  // Keep moving until limit switch is triggered
    digitalWrite(stepPin, HIGH);                                                       // Step pulse
    delayMicroseconds(stepInterval);                                                   // Control speed
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepInterval);
  }

  // Move forward to set zero position
  digitalWrite(dirPin, CLOCKWISE);  // Set motor direction to forward
  // first, move forward to disengage limit switch
  stepCounter = 0; // temporary value until we move forward to real zero
  while (( digitalRead(limit1Pin) == LOW || stepCounter < backlashSteps) && digitalRead(stopresetButtonPin) == HIGH) {  // Keep moving until limit switch is triggered
    digitalWrite(stepPin, HIGH);                                                      // Step pulse
    delayMicroseconds(stepInterval);                                                  // Control speed
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepInterval);
    stepCounter++;
  }

  // now, move forward to startPosition
  stepCounter = 0; // this is the real zero point
  while (stepCounter < steps_offset && digitalRead(stopresetButtonPin) == HIGH) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepInterval);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepInterval);
    stepCounter++;
  }
  position = stepsPosition(stepCounter);  //ensures the motor's position is calculated dynamically based on its actual step count
  // we are now homed, and the position should be startPosition
}

void setup() {
  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MonoNIR: og");
  delay(INITDELAY);

  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(startnextButtonPin, INPUT_PULLUP);
  pinMode(stopresetButtonPin, INPUT_PULLUP);
  pinMode(limit1Pin, INPUT_PULLUP);
  pinMode(autoManualPin, INPUT_PULLUP);

  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Set microstepping to 1/8 (M0 = HIGH, M1 = HIGH, M2 = LOW)
  digitalWrite(M0, HIGH);
  digitalWrite(M1, HIGH);
  digitalWrite(M2, LOW);


  // Ask if user wants to home
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Home?");
  lcd.setCursor(0, 1);
  lcd.print("Start=Y Stop=N");

  while (isFirstStart) {
    if (digitalRead(startnextButtonPin) == LOW) {  // User presses Start/Next for Yes
      delay(HOLDOFF);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Homing...");
      homeMotor();
      lcd.setCursor(0, 1);
      lcd.print("Homing Complete");
      // position = startPosition; // should be set in homeMotor()
      isFirstStart = false;
      isHomed = true;
      delay(INITDELAY);
      break;
    }
    if (digitalRead(stopresetButtonPin) == LOW) {  // User presses Stop/Reset for No
      delay(HOLDOFF);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Skipping Home");
      // some question here as to whether to set stepCounter to steps_offset, or change steps_offset to zero if we don't home
      stepCounter = steps_offset;
      position = startPosition;
      delay(INITDELAY);
      isFirstStart = false;
      break;
    }
  }



  /*lcd.setCursor(0, 1);
  lcd.print("Ready");
*/
  delay(STARTDELAY);
}

void loop() {
  // the following boolean flags are 1 if true, even if representing active-low signal
  bool upButton, downButton, startnextButton, stopresetButton, limit1, autoManual;
  long position_target;
  long steps_target;
  int cycles_completed=0;
  int resetval=0;

  if(hardReset==true) { // First start, or user has commanded a hard reset
    autoscan_Start=startPosition;
    autoscan_End=position_Min;
    autoscan_Speed=speed_Max;
    autoscan_Cycles=1;
    speed_nm_min=0;
    hardReset=false;
  }

  autoManual=!digitalRead(autoManualPin); // autoManual is true (switch closed) for manual mode
  if (autoManual == true) {  // Manual mode
    // speed selection for manual mode
    if(speed_nm_min==0) { // Manual speed has not been set, or a reset was commanded
      speed_nm_min=speed_Max;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Speed: ");
      // lcd.print(" nm/min");
      while( autoManual == true && digitalRead(startnextButtonPin)==HIGH ) {
        if (digitalRead(upButtonPin)==LOW){
          speed_nm_min +=speed_incr;//increase speed
          if(speed_nm_min>speed_Max){
            speed_nm_min -=speed_incr;
          }
        }
        if (digitalRead(downButtonPin)==LOW){
          speed_nm_min-=speed_incr;
          if(speed_nm_min<speed_Min){
            speed_nm_min+=speed_incr;
          }
        }
        // display the updated speed
        lcd.setCursor(6,0);
        lcd.print(speed_nm_min);
        lcd.print(" nm/min");
        lcd.write(" ");
        delay(HOLDOFF);
        // Check for mode change
        autoManual = !digitalRead(autoManualPin);
        // Check for reset
        if(digitalRead(stopresetButtonPin) == LOW) 
          resetval=resetHandler(stopresetButtonPin);
        if(resetval==2)
          goto bailout;
        if(resetval==1) 
          break;     
      } // end manual speed selection loop
    }
    else { // display current speed briefly
      lcd.clear();
      /*lcd.setCursor(0,0);
      lcd.print("Speed:");
      lcd.setCursor(6,0);
        lcd.print(speed_nm_min);
        lcd.print(" nm/min");
        lcd.write(" ");
        delay(HOLDOFF);
      delay(STARTDELAY);*/

    }

    // position selection for manual mode
    position_target = positionRound(position) ; // Start manual mode from our current position
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Manual,tgt=");
    lcd.print(position_target);
    lcd.write(' ');
    lcd.setCursor(0, 1);
    lcd.print(position);
    lcd.write("nm");
    lcd.setCursor(8, 1);
    lcd.print(stepCounter);
    lcd.write("st");
    // while loop for setting up the target position
    while( autoManual == true && digitalRead(startnextButtonPin)==HIGH ) {
      lcd.setCursor(11, 0);
      lcd.print(position_target);
      lcd.write(' ');  // Clear extra characters
      delay(HOLDOFF);

      // Adjust target wavelength with buttons
      if (digitalRead(upButtonPin) == LOW) {
        position_target += position_incr;  // Increase by 10 nm
        // check if this will be a problem
        if (position_target > position_Max || positionSteps(position_target) < steps_Min) {
          position_target -= position_incr;  // if a problem, undo change
        }
      }
      if (digitalRead(downButtonPin) == LOW) {
        position_target -= position_incr;  // Decrease by 10 nm
        // check if this will be a problem
        if (position_target < position_Min || positionSteps(position_target) > steps_Max) {
          position_target += position_incr;  // if a problem, undo change
        }
      }

      // Check for mode change
      autoManual = !digitalRead(autoManualPin);
      // Check for reset
      if(digitalRead(stopresetButtonPin) == LOW) 
        resetval=resetHandler(stopresetButtonPin);
      if(resetval==2)
        goto bailout;
      if(resetval==1){
        speed_nm_min=0; // trigger speed selection on next loop
        break;
      }
    } // end manual position selection loop

    // If we are still in manual mode and position_target is different from current position:
    if (autoManual == true && position_target != position) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tgt:");
      lcd.print(position_target);
      lcd.print("nm ");
      steps_target = positionSteps(position_target);  // Get steps for target wavelength
      lcd.print(steps_target);
      lcd.write("st ");
      delay(HOLDOFF); // Wait so user can see the target wavelength / steps for debugging (manual mode)

      // extra delay calculation for speed selection, c1 is steps per nm from calibration
      slow_us=(long)max((1000000.0 / (((double)speed_nm_min / 60) * abs(c1))) - (2.0 * (double)stepInterval), 0); 

      // Move motor to target steps
      if (steps_target > stepCounter) {  // move forward
        digitalWrite(dirPin, CLOCKWISE);
        // ABG: Use do ... while here so we can check that stopreset hasn't been pushed
        do {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(stepInterval);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(stepInterval);
          delayMicroseconds(slow_us);
          stepCounter++;
          // display current position: leaving option to not do this every step in case that makes it too slow
          if(stepCounter % 8 == 0) {
            displayPosition();
          }
          if(digitalRead(stopresetButtonPin) == LOW) 
            resetval=resetHandler(stopresetButtonPin);
          if(resetval==2)
            goto bailout;
          if(resetval==1)
            break;  
        } while (stepCounter < steps_target);
      } else if (steps_target < stepCounter) {  // move backward, will require backlash correction
        digitalWrite(dirPin, COUNTERCLOCKWISE);
        // ABG: Use do ... while here so we can check that stopreset hasn't been pushed
        do {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(stepInterval);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(stepInterval);
          delayMicroseconds(slow_us);
          stepCounter--;
          // display current position: leaving option to not do this every step in case that makes it too slow
          if(stepCounter % 8 == 0) {
            displayPosition();
          }
          if(digitalRead(stopresetButtonPin) == LOW) 
            resetval=resetHandler(stopresetButtonPin);
          if(resetval==2)
            goto bailout;
          if(resetval==1)
            break;  
        } while (stepCounter > steps_target);
        applyBacklashCorrection();
      } 

      // Synchronize stepCounter and position with the new target
      // stepCounter = targetSteps; // ABG: this is updated as we move, above
      position = stepsPosition(stepCounter);
    } // end of manual movement 
  } // end of manual segment


  if (autoManual == false) {  // Auto mode: switch open

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auto mode setup");

    delay(INITDELAY);

    // set Auto Mode parameters: autoscan_Start, autoscan_End, autoscan_Speed, autoscan_Cycles 
    // set autoscan_Start
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start Pos=");
    lcd.print(autoscan_Start);
    lcd.write(' ');
    lcd.setCursor(0, 1);
    lcd.print(position);
    lcd.write("nm");
    lcd.setCursor(8, 1);
    lcd.print(stepCounter);
    lcd.write("st");
    position_target=positionRound(autoscan_Start);
    // do while loop for setting up autoscan_Start
    do {
      lcd.setCursor(10, 0);
      lcd.print(position_target);
      lcd.write(' ');  // Clear extra characters
      delay(HOLDOFF);
      // Adjust target with buttons
      if (digitalRead(upButtonPin) == LOW) {
        position_target += position_incr;  // Increase by 10 nm
        // check if this will be a problem
        if (position_target > position_Max || positionSteps(position_target) < steps_Min) {
          position_target -= position_incr;  // if a problem, undo change
        }
      }
      if (digitalRead(downButtonPin) == LOW) {
        position_target -= position_incr;  // Decrease by 10 nm
        // check if this will be a problem
        if (position_target < position_Min || positionSteps(position_target) > steps_Max) {
          position_target += position_incr;  // if a problem, undo change
        }
      }
      // Check for mode change
      autoManual = !digitalRead(autoManualPin);
      if (autoManual == true) { // user switched to manual
        goto bailout;
      }
    } while (digitalRead(startnextButtonPin) == HIGH);
    autoscan_Start=position_target;

    // set autoscan_End
    if(autoscan_End>autoscan_Start){
       autoscan_End=autoscan_Start;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("End Pos=");
    lcd.print(autoscan_End);
    lcd.write(' ');
    lcd.setCursor(0, 1);
    lcd.print(position);
    lcd.write("nm");
    lcd.setCursor(8, 1);
    lcd.print(stepCounter);
    lcd.write("st");
    position_target=autoscan_End;
    delay(STARTDELAY); // waiting to make sure previous "start/next" button press has cleared
    // do while loop for setting up autoscan_End
    do {
      lcd.setCursor(8, 0);
      lcd.print(position_target);
      lcd.write(' ');  // Clear extra characters
      delay(HOLDOFF);
      // Adjust target with buttons
      if (digitalRead(upButtonPin) == LOW) {
        position_target += position_incr;  // Increase by 10 nm
        // check if this will be a problem
        if (position_target > autoscan_Start || positionSteps(position_target) < steps_Min) {
          position_target -= position_incr;  // if a problem, undo change
        }
      }
      if (digitalRead(downButtonPin) == LOW) {
        position_target -= position_incr;  // Decrease by 10 nm
        // check if this will be a problem
        if (position_target < position_Min || positionSteps(position_target) > steps_Max) {
          position_target += position_incr;  // if a problem, undo change
        }
      }
      // Check for mode change
      autoManual = !digitalRead(autoManualPin);
      if (autoManual == true) { // user switched to manual
        goto bailout;
      }
    } while (digitalRead(startnextButtonPin) == HIGH);
    autoscan_End=position_target;
       
    // set Auto mode speed
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Speed:");
    //lcd.print(autoscan_Speed);
    //lcd.print(" nm/min");
    lcd.write(" ");
   

    do {
      lcd.setCursor(6,0);
      lcd.print(autoscan_Speed);
      lcd.print(" nm/min");
      lcd.write(" ");
      delay(HOLDOFF);
      
      // adjust speed with up and down buttons
      if (digitalRead(upButtonPin)==LOW){
        autoscan_Speed+=speed_incr; //increase speed
        if(autoscan_Speed > speed_Max){
          autoscan_Speed-=speed_incr; //undo if over max speed
        }
      }
      if(digitalRead(downButtonPin)==LOW){
        autoscan_Speed-=speed_incr; // decrease speed
        if (autoscan_Speed < speed_Min){;
          autoscan_Speed+=speed_incr; //undo if below min speed
        }
      }
      // check for mode change
      autoManual=!digitalRead(autoManualPin);
      if (autoManual==true){
        goto bailout;
      }
    } while (digitalRead(startnextButtonPin)==HIGH);

    // Calculate extra delay (slow_us) to match the selected speed in nm/min. c1 is from calibration
    slow_us=(long)max((1000000.0 / (((double)autoscan_Speed / 60) * abs(c1))) - (2.0 * (double)stepInterval), 0); 
    // Ensures total step time (2 * stepInterval + slow_us) aligns with user-defined speed.

    // Main auto loop: waits for user to press start and then runs scan
    while (autoManual == false) {  // Main loop
      // move to start
      steps_target=positionSteps(autoscan_Start);
      if (steps_target > stepCounter) {  // move forward
        digitalWrite(dirPin, CLOCKWISE);
        // ABG: Use do ... while here to check that stopreset hasn't been pushed
        do {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(stepInterval);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(stepInterval); // move to start position at max speed
          stepCounter++;
          // display current position: leaving option to not do this every step in case that makes it too slow
          if(stepCounter % 8 == 0) {
            displayPosition();
          }
          if(digitalRead(stopresetButtonPin) == LOW) 
            resetval=resetHandler(stopresetButtonPin);
          if(resetval==2)
            goto bailout;
          if(resetval==1)
            break;  
        } while (stepCounter < steps_target);
      } else if (steps_target < stepCounter) {  // move backward, will require backlash correction
        digitalWrite(dirPin, COUNTERCLOCKWISE);
        // ABG: Use do ... while here to check that stopreset hasn't been pushed
        do {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(stepInterval);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(stepInterval); // move to start position at max speed
          stepCounter--;
          // display current position: leaving option to not do this every step in case that makes it too slow
          if(stepCounter % 8 == 0) {
            displayPosition();
          }
          if(digitalRead(stopresetButtonPin) == LOW) 
            resetval=resetHandler(stopresetButtonPin);
          if(resetval==2)
            goto bailout;
          if(resetval==1)
            break;  
        } while (stepCounter > steps_target);
        applyBacklashCorrection();
      }
      position = stepsPosition(stepCounter);
      // Alert user to press start
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rdy:");
      lcd.print(autoscan_Start);
      lcd.print(" to:");
      lcd.print(autoscan_End);
      lcd.write(" ");
      lcd.setCursor(0, 1);
      lcd.print(position);
      lcd.write("nm");
      lcd.setCursor(8, 1);
      lcd.print(stepCounter);
      lcd.write("st");
      delay(STARTDELAY); // waiting to make sure previous "start/next" button press has cleared
      while(digitalRead(startnextButtonPin)==HIGH && digitalRead(stopresetButtonPin)==HIGH){
        autoManual = !digitalRead(autoManualPin);
        if (autoManual == true) { // user switched to manual
          goto bailout;
        }
      }
      cycles_completed=0;
      // Scan loop: runs the requested number of scans, comes back to auto Main loop
      while(cycles_completed<autoscan_Cycles && digitalRead(stopresetButtonPin)==HIGH){ // Scan loop
        // move forward, turn on LED / start signal
        steps_target=positionSteps(autoscan_End);
        digitalWrite(ledPin,HIGH);
        digitalWrite(dirPin, CLOCKWISE);
        do {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(stepInterval);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(stepInterval);
          delayMicroseconds(slow_us); // scan at programmed speed
          stepCounter++;
          // display current position: leaving option to not do this every step in case that makes it too slow
          if(stepCounter % 8 == 0) {
            displayPosition();
          }
          if(digitalRead(stopresetButtonPin) == LOW) 
            resetval=resetHandler(stopresetButtonPin);
          if(resetval==2)
            goto bailout;
          if(resetval==1) {
            cycles_completed=autoscan_Cycles;
            break;  
          }
        } while (stepCounter < steps_target);
        digitalWrite(ledPin,LOW);
        // move backward
        steps_target=positionSteps(autoscan_Start);
        digitalWrite(dirPin, COUNTERCLOCKWISE);
        do {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(stepInterval);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(stepInterval);  // return at max speed
          stepCounter--;
          // display current position: leaving option to not do this every step in case that makes it too slow
          if(stepCounter % 8 == 0) {
            displayPosition();
          }
          if(digitalRead(stopresetButtonPin) == LOW) 
            resetval=resetHandler(stopresetButtonPin);
          if(resetval==2)
            goto bailout; 
          if(resetval==1) {
            cycles_completed=autoscan_Cycles;
            break;  
          }
        } while (stepCounter > steps_target);
        applyBacklashCorrection();
        cycles_completed++;
        // Check for mode change
        autoManual = !digitalRead(autoManualPin);
        if (autoManual == true) { // user switched to manual
          goto bailout;
        }
      } // close auto scan loop: start next scan
      lcd.setCursor(0, 1);
      if(resetval==0) {
        lcd.print("Scans Complete");
      }
      else {
        lcd.print("Scans Aborted");
        resetval=0; // clear error
      }
      delay(INITDELAY);
      } // close main loop: should wait for user to start another auto scan sequence

    } // close auto mode if statement

  // note the code below gets run even if we aren't sent to bailout by goto
  bailout:
  displayPosition();
  if(resetval==2) { // hard reset: restore default scan parameters
    hardReset=1;
  }
  resetval=0; // clear errors
  delay(STARTDELAY);
} // close loop()
