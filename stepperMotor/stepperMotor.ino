//Stepper motor + vent
#include <Stepper.h> // Include the header file


// change this to the number of steps on your motor

#define STEPS 32


// create an instance of the stepper class using the steps and pins

Stepper stepper(STEPS, 10, 12, 11, 13);

const int buttonPin = 23;
const int buttonPin2 = 24;
int buttonState = 0; 
int buttonState2 = 0; 



void setup() {

  Serial.begin(9600);

  stepper.setSpeed(1100);
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin2, INPUT);

}


void loop() {
  buttonState = digitalRead(buttonPin);
  buttonState2 = digitalRead(buttonPin2);

  if (buttonState == HIGH) {
    // turn LED on:
      stepper.step(50);
  } 
  if(buttonState2 == HIGH)
  {
    // turn LED off:
    stepper.step(-50);
  }
  else
  {
    stepper.step(0);
  }

}
