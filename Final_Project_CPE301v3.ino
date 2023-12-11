//Authors: Fenn Edmonds, Liam Francisco, Zack Chotinun, Ashley Eubanks
#include <dht.h>
#include <Stepper.h> // Include the header file
//#include <Wire.h> // for the ISR timer
//#include <RTClib.h>

#define STEPS 32

dht DHT;
#define DHTPIN A4

#define RDA 0x80
#define TBE 0x20  

//RTC_DS3231 rtc; //real time clock
//volatile bool secondFlag = false;

Stepper stepper(STEPS, 10, 12, 11, 13);

const int buttonPin2 = 23;
const int buttonPin3 = 24;
const int buttonPin4 = 25;
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
//LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

int lowerThreshold = 420;
int upperThreshold = 520;
bool resetButton = true;

bool disabled = true; //keeps track of the state of the system
unsigned int currentTicks;
//bool error = false; //keeps track if the system is in ERROR mode

void setup() 
{
  // setup the UART
  U0init(9600);
  // setup the ADC
  adc_init();

  //stepper motor
  stepper.setSpeed(1100);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  
  //setting up LED's for water ouput reading
  pinMode(A3, OUTPUT); //Motor Driver - IN2
  pinMode(A2, OUTPUT); //Motor Driver - IN3
  pinMode(A1, OUTPUT); //Motor Driver - ENA
  pinMode(6, OUTPUT); //red LED
  pinMode(7, OUTPUT); //yellow LED
  pinMode(8, OUTPUT); //green LED
  pinMode(9, OUTPUT); //blue LED

  //lcd.begin(16, 2); // set up number of columns and rows

  pinMode(buttonPin4, INPUT);
  
//  // Timer 
//  Wire.begin();
//
//  if (!rtc.begin()) {
//    Serial.println("RTC Error");
//    while (1);
//  }
 
}
void loop() 
{
  buttonState2 = digitalRead(buttonPin2);
  buttonState3 = digitalRead(buttonPin3);

  if (buttonState2 == HIGH) {
    // turn LED on:
      stepper.step(50);
  } 
  if(buttonState3 == HIGH)
  {
    // turn LED off:
    stepper.step(-50);
  }
  else
  {
    stepper.step(0);
  }
 buttonState = digitalRead(buttonPin);
  if(disabled == true)
  {
    if (buttonState == HIGH)
  {
    currentTicks = 0;
    disabled = false;
  }
  digitalWrite(3, LOW);
  digitalWrite(7, HIGH); //In disabled state the yellow LED should be on 
  )
  
  if(disabled == false)
  {

    if(buttonState == HIGH)
    {
      currentTicks = 65535;
    }
    
    digitalWrite(7, LOW); // yellow LED off

    //controls motor direction
    digitalWrite(A3, LOW);
    digitalWrite(A2, HIGH);

    int readDHT = DHT.read11(DHTPIN);
    float temp = DHT.temperature;
    float humid = DHT.humidity;

    unsigned int input;
    unsigned int input1 = 0;
    unsigned int input2 = 0;
    unsigned int input3 = 0;
    unsigned int input4 = 0;
    input = adc_read(0); //read water level

    if(input >= 1000){
      input1 = input / 1000 + '0';
      U0putchar(input / 1000 + '0');
      input = input % 1000;
    }

    if(input >= 100){
      input2 = input / 100 + '0';
      U0putchar(input / 100 + '0');
      input = input % 100;
    }

    if(input >= 10){
      input3 = input / 10 + '0';
      U0putchar(input / 10 + '0');
      input = input % 10;
    }

    U0putchar(input + '0');
    input4 = input + '0';
    Serial.print ("\n");

    if(input3 < 1 || input3 == 0 && input2 < 6) //ERROR MODE
    {
      digitalWrite(8, LOW); // green LED off
      digitalWrite(9, LOW); //bluee LED off
      digitalWrite(6, HIGH); // red LED on
      digitalWrite(3, LOW); //turn off fan
      resetButton = false;
      while(resetButton == false)
      {
        buttonState4 = digitalRead(buttonPin4);
        if(buttonState4 == HIGH && input3 > 1 || input3 == 0 && input2 > 6)
        {
          resetButton = true;
        }
        else{
          digitalWrite(6, HIGH); // red LED on
          //low water lvl (print to screen)
          //ERROR MODE intiated
          //lcd.setCursor(0, 0);
          //lcd.print("Water level is too low"); 
        }
      }
    }

    if(temp < 23) //RUNNING MODE
    {
      digitalWrite(6, LOW); // red led off
      digitalWrite(8, LOW); //green led off
      digitalWrite(9, HIGH); //bluee LED off
      digitalWrite(3, HIGH); //turn on fan
    }

    else //IDLE MODE
    {
      digitalWrite(3, LOW); //turn off fan
      digitalWrite(9, LOW); //blue LED off
      digitalWrite(6, LOW);
      digitalWrite(8, HIGH); // green LED on
 
      //medium/high water lvl
    }
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
    disbaled = false;
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
