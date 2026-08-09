// Very basic demo of LCD display and use of pushbuttons to increment and decrement a value
#include <LiquidCrystal.h>

const int lcdRSPin = 2;
const int lcdEPin = 3;
const int lcdD4Pin = 4;
const int lcdD5Pin = 5;
const int lcdD6Pin = 6;
const int lcdD7Pin = 7;

// Pins A0 thru A5, which can be used as analog inputs, are used here as digital inputs.
// You can do that, and pins A0-A6 are digital pins 15-19 when configured as digital inputs.
// I think you can also pass "A0" etc to pinMode, but doing it this way makes it clearer 
// that we are not collecting analog signals right now.
const int upButtonPin=14; 
const int downButtonPin=15;

const int HOLDOFF = 150; // milliseconds after button press before we read again
const int ledPin=LED_BUILTIN;
//int ledPin=13;


int displayValue = 50;

LiquidCrystal lcd(lcdRSPin, lcdEPin, lcdD4Pin, lcdD5Pin, lcdD6Pin, lcdD7Pin);

void setup()
{
  // set up the LCD's number of columns and rows
  lcd.begin(16,2);

  // print startup message
  lcd.clear();
  lcd.print("Hello, world!");
  lcd.setCursor(0,1);
  lcd.print(displayValue);
  pinMode(upButtonPin,INPUT_PULLUP);
  pinMode(downButtonPin,INPUT_PULLUP);
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin,LOW);
  delay(HOLDOFF);
}

void loop()
{
  // lcd.setCursor(0,1);
  // lcd.print(millis() / 1000);
  int pinval;
//  digitalWrite(ledPin,LOW);
  if(digitalRead(upButtonPin)==LOW) // signal is active-low
  {
    displayValue++;
    digitalWrite(ledPin,HIGH);
    delay(HOLDOFF);
    digitalWrite(ledPin,LOW);
  //  delay(100);
  }
  if(digitalRead(downButtonPin)==LOW) // signal is active-low
  {
    displayValue--;
    digitalWrite(ledPin,HIGH);
    delay(HOLDOFF);
    digitalWrite(ledPin,LOW);
  //  delay(100);
  }

  lcd.setCursor(0,1);
  lcd.print(displayValue);  // note this simply prints the number, and even works for negative numbers!
  lcd.write('   '); // write spaces after the number to be sure to cover any previous digits

}


