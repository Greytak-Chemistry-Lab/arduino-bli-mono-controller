#include <LiquidCrystal.h>

// Define pin numbers for LCD display
const int lcdRSPin = 2;
const int lcdEPin = 3;
const int lcdD4Pin = 4;
const int lcdD5Pin = 5;
const int lcdD6Pin = 6;
const int lcdD7Pin = 7;

// Direction definitions
#define CLOCKWISE 1
#define COUNTERCLOCKWISE 0

// Motor control and microstepping pins
const int dirPin = 8;
const int stepPin = 9;
const int M0 = 11;
const int M1 = 12;
const int M2 = 13;

// Button and limit switch pins
const int forwardButton = 14;
const int backwardButton = 15;
const int limit1 = 18;
const int limit2 = 19;

// Motor settings
const int stepsPerRevolution = 1600;   // 1/8 microstepping
const unsigned long stepInterval = 12500;  // 12500 microseconds for 80 steps per second

// Initialize variables
LiquidCrystal lcd(lcdRSPin, lcdEPin, lcdD4Pin, lcdD5Pin, lcdD6Pin, lcdD7Pin);
int motorDirection = CLOCKWISE;
bool motorStopped = true;
int position = 900;  // Start position in nm
bool isFirstStart=true;// flag to indicate the very first start

unsigned long lastStepTime = 0;  // To track step timing
unsigned long lastLCDUpdate = 0; // To track LCD update timing
int stepCounter=0;

void setup() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stepper V1.0");
  delay(3000);

  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);

  pinMode(forwardButton, INPUT_PULLUP);
  pinMode(backwardButton, INPUT_PULLUP);
  pinMode(limit1, INPUT_PULLUP);
  pinMode(limit2, INPUT_PULLUP);

  // Set microstepping to 1/8 (M0 = HIGH, M1 = HIGH, M2 = LOW)
  digitalWrite(M0, HIGH);
  digitalWrite(M1, HIGH);
  digitalWrite(M2, LOW);

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Ready");
  
}

void loop() {
  // Check forward button to set direction
  if (digitalRead(forwardButton) == LOW) {
    motorDirection = CLOCKWISE;
    motorStopped = false;

    if (isFirstStart) {
    position = 900;    // Reset position to 900 nm when starting
    isFirstStart=false; // Now it's no longer the first start
    }
   
    delay(150);    // Debounce delay
      
  }
  // Check backward button to set direction
  else if (digitalRead(backwardButton) == LOW) {
    motorDirection = COUNTERCLOCKWISE;
    motorStopped = false;

    if (isFirstStart){
      position=900;
      isFirstStart=false;
    }
    delay(150);    // Debounce delay
    
  }

  // Check limit switches to stop motor
  if (digitalRead(limit1) == LOW || digitalRead(limit2) == LOW) {
    motorStopped = true;
    delay(150);       // Debounce delay for limit switch
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stopped");
    lcd.setCursor(0, 1);
lcd.print("Position:");
lcd.print(position);  // Display the current position value
lcd.print(" nm"); // Clear any extra characters
  }

  // Non-blocking motor stepping logic
  if (!motorStopped) {
    unsigned long currentTime = micros();

    if (currentTime - lastStepTime >= stepInterval) { // This calculates the time since the motor last stepped and If enough time has passed since the last step.
      lastStepTime = currentTime;  // Update last step time

      digitalWrite(dirPin, motorDirection == CLOCKWISE ? LOW : HIGH);  // Set direction (LOW for CLOCKWISE)
      digitalWrite(stepPin, HIGH);  // Pulse step pin
      delayMicroseconds(10);        // Short pulse
      digitalWrite(stepPin, LOW);
stepCounter++;
    }
  }

  // Update position and LCD every 80 steps (1 second of movement)
  if (stepCounter >= 80) {
    stepCounter = 0;  // Reset counter after updating position

    // Update position based on direction
    if (motorDirection == CLOCKWISE) {
      position += 10;  // Increment by 10 nm every 80 steps in clockwise
    } else {
      position -= 10;  // Decrement by 10 nm every 80 steps in counterclockwise
    }

    // LCD Updates
    lcd.setCursor(0, 0);
    lcd.print(motorDirection == CLOCKWISE ? "Forward       " : "Backward      ");
    lcd.setCursor(0, 1);
    lcd.print("Position:");
    lcd.print(position);
    lcd.print(" nm");  // Extra spaces to clear old characters
  }
}
