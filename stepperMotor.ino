//Stepper motor + vent

#include <Stepper.h>
#define STEPS 32

Stepper stepper(STEPS, 10, 12, 11, 13);

int Pval = 0;
int potVal = 0;

void setup() {

  Serial.begin(9600);

  stepper.setSpeed(200);

}

void loop() {

  potVal = map(analogRead(A1),0,1024,0,500);

  if (potVal>Pval)
  {
    stepper.step(5);
  }

  if (potVal<Pval)
  {
    stepper.step(-5);
  }

  Pval = potVal;

  //Serial.println(Pval); //for debugging

}
