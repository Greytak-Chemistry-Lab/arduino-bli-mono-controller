// A first attempt at using buttons, limit switch, and LCD to control stepper motor
#include <LiquidCrystal.h>
#include <AccelStepper.h>  // Include the AccelStepper library for easier motor control

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
const int autoManualPin = 19; // LOW (switch closed) = auto

const int HOLDOFF = 150; // milliseconds after button press before we read again
const int INITDELAY = 2000; // delay on setup()
const int STARTDELAY = 500; // delay after next before accepting start signal
const int CYCLEDELAY = 50; // for polling button presses and limit switches while moving

// Define LED pin
const int ledPin = LED_BUILTIN;

// Motor control and microstepping pins
const int dirPin = 8;
const int stepPin = 9;
const int M0 = 10;
const int M1 = 11;
const int M2 = 12;

// Motor settings
const int stepsPerRevolution = 1600;   // 1/8 microstepping

// Direction definitions
#define CLOCKWISE 1
#define COUNTERCLOCKWISE 0

// Polynomial constants from calibration
float c0 = 0;
float c1 = 8;
float c2 = 0;
float c3 = 0;

long steps_offset = 80;          // Offset after homing
long stepCounter = 0;            // Tracks current step position from home
long startPosition = 900;        // Start position in nm
long position;                   // current position in nm
bool isFirstStart = true;        // Flag to indicate the very first start

// Initialize LCD and AccelStepper
LiquidCrystal lcd(lcdRSPin, lcdEPin, lcdD4Pin, lcdD5Pin, lcdD6Pin, lcdD7Pin);
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin); // Initialize stepper as DRIVER type (for step/direction control)

// Function to calculate motor steps based on target wavelength
int positionSteps(long position_requested) {
    double prf, steps;
    prf = (double)(position_requested - startPosition); 
    steps = c0 + c1 * prf + c2 * pow(prf, 2) + c3 * pow(prf, 3) + steps_offset;
    return (long)steps;
}

// Function to perform homing and set steps_offset
void homeMotor() {
    stepper.setSpeed(-80);  // Set speed in COUNTERCLOCKWISE direction for homing

    // Move until limit switch is triggered
    while (digitalRead(limit1Pin) == HIGH && digitalRead(stopresetButtonPin) == HIGH) {
        stepper.runSpeed();  // runSpeed keeps moving the motor at the set speed without acceleration
    }

    // Move forward slightly to disengage limit switch
    stepper.setSpeed(80);   // Set speed in the opposite direction to move forward
    while (digitalRead(limit1Pin) == LOW && digitalRead(stopresetButtonPin) == HIGH) {
        stepper.runSpeed();  // runSpeed continues to move the motor at the new speed
    }

    // Offset position by `steps_offset`
    stepper.move(steps_offset);  // move steps_offset steps forward from the home position
    while (stepper.distanceToGo() != 0) {  // distanceToGo returns the remaining steps to target
        stepper.run();  // run gradually moves motor to the target position with acceleration/deceleration
    }
    stepCounter = steps_offset;
    position = startPosition;
}

void setup() {
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stepper Basic");
    delay(INITDELAY);

    // Configure input pins
    pinMode(upButtonPin, INPUT_PULLUP);
    pinMode(downButtonPin, INPUT_PULLUP);
    pinMode(startnextButtonPin, INPUT_PULLUP);
    pinMode(stopresetButtonPin, INPUT_PULLUP);
    pinMode(limit1Pin, INPUT_PULLUP);
    pinMode(autoManualPin, INPUT_PULLUP);
  
    // Configure microstepping pins
    pinMode(M0, OUTPUT);
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    digitalWrite(M0, HIGH);
    digitalWrite(M1, HIGH);
    digitalWrite(M2, LOW);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // AccelStepper configuration
    stepper.setMaxSpeed(50);       // Set max speed in steps per second
    stepper.setAcceleration(50);   // Set acceleration in steps per second squared

    lcd.setCursor(0, 1);
    lcd.print("Ready");
    delay(STARTDELAY);
}

void loop() {
    bool autoManual = !digitalRead(autoManualPin);
    int position_target = position;

    // Perform homing on first start
    if (isFirstStart) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Initializing...");
        homeMotor();
        lcd.setCursor(0, 1);
        lcd.print("Homing Complete");
        isFirstStart = false;
        delay(INITDELAY);
    }

    if (autoManual == false) { // Manual mode
        position_target = 900;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Manual, tgt=");
        lcd.print(position_target);
        lcd.write(' ');
        lcd.setCursor(0, 1);
        lcd.print(position);

        // do-while loop for setting up the target position
        do {
            lcd.setCursor(11, 0);
            lcd.print(position_target);
            lcd.write(' ');  // Clear extra characters
            delay(HOLDOFF);

            if (digitalRead(upButtonPin) == LOW) {
                position_target += 10;
            }
            if (digitalRead(downButtonPin) == LOW) {
                position_target -= 10;
            }

            autoManual = !digitalRead(autoManualPin);
            if (autoManual == true) {
                break;
            }
        } while (digitalRead(startnextButtonPin) == HIGH);

        if (autoManual == false && position_target != position) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Moving!");

            if (digitalRead(startnextButtonPin) == LOW) {
                delay(50);

                int steps_target = positionSteps(position_target);
                stepper.moveTo(steps_target);  // Set the target position in steps using moveTo

                while (stepper.distanceToGo() != 0) {  // While there are steps left to reach the target
                    stepper.run();  // run moves the motor gradually towards the target
                    if (digitalRead(stopresetButtonPin) == LOW) {
                        stepper.stop();  // stop immediately stops the motor if reset is pressed
                        break;
                    }
                }

                position = position_target;  // Update position to the new target
            }
        }
    }

    if (autoManual == true) {  // Auto mode
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Auto mode");

        int steps_target = positionSteps(position_target);
        stepper.moveTo(steps_target);  // Set the target position for automatic movement

        while (stepper.distanceToGo() != 0) {  // Move until the target position is reached
            stepper.run();  // Gradually moves motor towards the target with acceleration
        }

        stepCounter = steps_target;
        position = position_target;
        delay(INITDELAY);
    }
}
