//Authors: Fenn Edmonds, Liam Francisco, Zack Chotinun, Ashley Eubanks
#include <dht.h>
#include <Stepper.h> // Include the header file
#include <Wire.h> // for the timer
#include <RTClib.h>
#include <LiquidCrystal.h>

#define STEPS 60

dht DHT;
#define DHTPIN 34

#define RDA 0x80
#define TBE 0x20  

RTC_DS3231 rtc; //real time clock
unsigned long lastHumTempCheck = 0;
const unsigned long humTempCheckInterval = 60000; // checks every 1 minute

Stepper stepper(STEPS, 10, 12, 11, 13);

const int buttonPin2 = 23;
const int buttonPin3 = 24;
const int buttonPin4 = 28;
int buttonState2 = 0; 
int buttonState3 = 0; 
int buttonState4 = 0;

const int buttonPin = 26;
int buttonState = 0; 

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Timer Pointers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

// LCD pins <--> Arduino pins
const int RS = 5, EN = 4, D4 = 3, D5 = 2, D6 = 1, D7 = 0;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

float temp;
float humid;

int waterThreshold = 100;

float temperatureThreshold = 30;
int waterLevel;
bool resetButton = true;

//state variables
bool disabled = false; //keeps track of the state of the system
bool idle = false;
bool running = false;
bool error = true;
unsigned int currentTicks;
//bool error = false; //keeps track if the system is in ERROR mode

void setup() 
{
  // setup the UART
  U0init(9600);
  // setup the ADC
  adc_init();

  pinMode(buttonPin, INPUT); //start and stop

  //stepper motor
  stepper.setSpeed(1100);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  
  
  pinMode(A3, OUTPUT); //Motor Driver - IN2
  pinMode(A2, OUTPUT); //Motor Driver - IN3
  pinMode(A1, OUTPUT); //Motor Driver - ENA

  //setting up LED's for water ouput reading
  pinMode(6, OUTPUT); //red LED
  pinMode(7, OUTPUT); //yellow LED
  pinMode(8, OUTPUT); //green LED
  pinMode(9, OUTPUT); //blue LED

  pinMode(buttonPin4, INPUT); //reset
  
  // Timer and LCD
  Wire.begin();
  lcd.begin(16, 2); // set up number of columns and rows
 
}
void loop() 
{
  //int readDHT = DHT.read11(DHTPIN);
  //temp = DHT.temperature;
  //Serial.println(temp);
  //waterLevel = adc_read(0);
  //Serial.println(waterLevel);
  if(disabled){
    digitalWrite(6, LOW); //red LED off
    digitalWrite(7, HIGH); //yellow LED on
    digitalWrite(8, LOW); //green LED off
    digitalWrite(9, LOW); //blue LED on
  }
  else if(idle){
    //read water level
    waterLevel = adc_read(0);

    //read temperature
    int readDHT = DHT.read11(DHTPIN);
    temp = DHT.temperature;
    Serial.println(temp);
    Serial.println(waterLevel);

    digitalWrite(6, LOW); //red LED off
    digitalWrite(7, LOW); //yellow LED off
    digitalWrite(8, HIGH); //green LED on
    digitalWrite(9, LOW); //blue LED on

    //idle to running check
    if(temp > temperatureThreshold){
      disabled = false;
      idle = false;
      running = true;
      error = false;
    }

    //idle to error check
    if(waterLevel <= waterThreshold){
      disabled = false;
      idle = false;
      running = false;
      error = true;
    }

    //idle to disabled check
    buttonState = digitalRead(buttonPin);
    if(buttonState == HIGH){
      disabled = true;
      idle = false;
      running = false;
      error = false;
    }
  }
  else if(running){
    //read water level
    waterLevel = adc_read(0);

    //read temperature
    int readDHT = DHT.read11(DHTPIN);
    temp = DHT.temperature;
    Serial.println(temp);
    Serial.println(waterLevel);

    //turn on fan
    digitalWrite(A1, HIGH);
    digitalWrite(A2, LOW);
    digitalWrite(A3, HIGH);

    digitalWrite(6, LOW); //red LED off
    digitalWrite(7, LOW); //yellow LED on
    digitalWrite(8, LOW); //green LED off
    digitalWrite(9, HIGH); //blue LED on

    //running to idle
    if(temp <= temperatureThreshold){
      disabled = false;
      idle = true;
      running = false;
      error = false;
      digitalWrite(A1, LOW);
    }

    //running to error check
    if(waterLevel <= waterThreshold){
      disabled = false;
      idle = false;
      running = false;
      error = true;
      digitalWrite(A1, LOW);
    }

    //running to disabled check
    buttonState = digitalRead(buttonPin);
    if(buttonState == HIGH){
      disabled = true;
      idle = false;
      running = false;
      error = false;
      digitalWrite(A1, LOW);
    }    
  }
  else if(error){
    digitalWrite(6, HIGH); //red LED on
    digitalWrite(7, LOW); //yellow LED off
    digitalWrite(8, LOW); //green LED off
    digitalWrite(9, LOW); //blue LED off

    //error to idle check
    buttonState4 = digitalRead(buttonPin4);
    if(buttonState4 == HIGH){
      disabled = false;
      idle = true;
      running = false;
      error = false;
    }

    //error to disabled check
    buttonState = digitalRead(buttonPin);
    if(buttonState == HIGH){
      disabled = true;
      idle = false;
      running = false;
      error = false;
    }
  }

  if(!error){
    buttonState2 = digitalRead(buttonPin2);
    buttonState3 = digitalRead(buttonPin3);


    if (buttonState2 == HIGH) {
      // turn LED on:
      //stepper.setSpeed(1);
      stepper.step(STEPS);
      //Serial.print("left");

      /*
      //displaying to LCD
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Stepper Motor:");
      lcd.setCursor(0,1);
      lcd.print("MOVED FORWARD");
      RTCdisplay(); //clears then displays date and time
      */

      //displaying to serial
      //Serial.print("Stepper Motor: MOVED FORWARD- ");
      //displaySerial();
    }
    else if(buttonState3 == HIGH){
      // turn LED off:
      //stepper.setSpeed(1);
      stepper.step(-STEPS); //this moves motor backwards???
      //Serial.print("right");
      /*
      //displaying to LCD
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Stepper Motor:");
      lcd.setCursor(0,1);
      lcd.print("MOVED BACKWARD");
      RTCdisplay(); //clears then displays date and time


      //displaying to serial
      Serial.print("Stepper Motor: MOVED BACKWARD- ");
      displaySerial();
      */
    }
    else{
      stepper.step(0);
    }
  }
}

ISR(TIMER1_OVF_vect)
{
  // Stop the Timer
  *myTCCR1B &=0xF8;
  // Load the Count
  *myTCNT1 =  (unsigned int) (65535 -  (unsigned long) (currentTicks));
  // Start the Timer
  *myTCCR1B |=   0x01;
  // if it's not the STOP amount
  if(currentTicks != 65535)
  {
    disabled = false;
  }
  else
    disabled = true;
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void RTCdisplay()
{
  lcd.clear();
  DateTime now= rtc.now();
  //date
  lcd.setCursor(0,0);
  lcd.print(now.day(), DEC);
  lcd.print('/');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);
  lcd.print(' ');

  //time
  lcd.setCursor(0,1);
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);

  //delay(1000);
}

void displaySerial()
{
  DateTime now = rtc.now();
  //date
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" ");

  //time
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  //delay(1000);
}

//converts value to string to shorten decimal place to ensure the value fits on lcd screen
String formatFloat(float value, int length)
{
  String shorter = String(value, length);
  return shorter;
}


