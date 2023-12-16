//Authors: Fenn Edmonds, Liam Francisco, Zack Chotinun, Ashley Eubanks
#include <dht.h>
#include <Stepper.h> // Include the header file
#include <Wire.h> // for the timer
#include <RTClib.h>
#include <LiquidCrystal.h>

#define STEPS 32

dht DHT;
#define DHTPIN 34

#define RDA 0x80
#define TBE 0x20  

RTC_DS3231 rtc;

Stepper stepper(STEPS, 43, 47, 45, 49);

DateTime now;

const int buttonPin = 26;
const int buttonPin2 = 23;
const int buttonPin3 = 24;
const int buttonPin4 = 28;
const int buttonPin5 = 18;
int buttonState = 0; 
int buttonState2 = 0; 
int buttonState3 = 0; 
int buttonState4 = 0;
int buttonState5 = 0;

volatile unsigned char* port_f = (unsigned char*) 0x31; 
volatile unsigned char* ddr_f  = (unsigned char*) 0x30; 
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;

volatile unsigned char* port_h = (unsigned char*) 0x102; 
volatile unsigned char* ddr_h  = (unsigned char*) 0x101; 
volatile unsigned char* pin_h  = (unsigned char*) 0x100;

volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
volatile unsigned char* pin_d  = (unsigned char*) 0x29;

volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a  = (unsigned char*) 0x21; 
volatile unsigned char* pin_a  = (unsigned char*) 0x20;

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
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

float temp;
float humid;

int waterThreshold = 100;

float temperatureThreshold = 22;
int waterLevel;
bool resetButton = true;
bool ventInUse = false;

//state variables
bool disabled = true; //keeps track of the state of the system
bool idle = false;
bool running = false;
bool error = false;

void setup() 
{
  // setup the UART
  U0init(9600);
  // setup the ADC
  adc_init();

  //pinMode(buttonPin, INPUT); //stop
  *ddr_a &= 0xEF;
  //pinMode(buttonPin5, INPUT); //start
  *ddr_d &= 0xF7;
  attachInterrupt(digitalPinToInterrupt(buttonPin5), startButton, CHANGE);

  //stepper motor
  stepper.setSpeed(500);
  //pinMode(buttonPin2, INPUT);
  *ddr_a &= 0xFD;
  //pinMode(buttonPin3, INPUT);
  *ddr_a &= 0xFB;
  
  
  //pinMode(A3, OUTPUT); //Motor Driver - IN2
  *ddr_f |= 0x08;
  //pinMode(A2, OUTPUT); //Motor Driver - IN3
  *ddr_f |= 0x04;
  //pinMode(A1, OUTPUT); //Motor Driver - ENA
  *ddr_f |= 0x02;

  //setting up LED's for water ouput reading
  //pinMode(6, OUTPUT); //red LED
  *ddr_h |= 0x08;
  //pinMode(7, OUTPUT); //yellow LED
  *ddr_h |= 0x10;
  //pinMode(8, OUTPUT); //green LED
  *ddr_h |= 0x20;
  //pinMode(9, OUTPUT); //blue LED
  *ddr_h |= 0x40;

  //pinMode(buttonPin4, INPUT); //reset
  *ddr_a &= 0xBF;
  
  // Timer and LCD
  Wire.begin();
  lcd.begin(16, 2); // set up number of columns and rows

  // automatically sets the RTC to the date & time on PC this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 
}
void loop() 
{
  now = rtc.now();

  if(disabled){
    //digitalWrite(6, LOW); //red LED off
    //digitalWrite(7, HIGH); //yellow LED on
    //digitalWrite(8, LOW); //green LED off
    //digitalWrite(9, LOW); //blue LED on
    setRed(false);
    setYellow(true);
    setGreen(false);
    setBlue(false);
  }
  else if(idle){
    //read water level
    waterLevel = adc_read(0);

    //read temperature
    int readDHT = DHT.read11(DHTPIN);
    temp = DHT.temperature;

    //digitalWrite(6, LOW); //red LED off
    //digitalWrite(7, LOW); //yellow LED off
    //digitalWrite(8, HIGH); //green LED on
    //digitalWrite(9, LOW); //blue LED off
    setRed(false);
    setYellow(false);
    setGreen(true);
    setBlue(false);

    //idle to running check
    if(temp > temperatureThreshold){
      disabled = false;
      idle = false;
      running = true;
      error = false;
      String change = "Changed from idle to running at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
      displayDHT();
    }

    //idle to error check
    if(waterLevel <= waterThreshold){
      disabled = false;
      idle = false;
      running = false;
      error = true;
      String change = "Changed from idle to error at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
    }

    //idle to disabled check
    if(*pin_a & 0x10){
      disabled = true;
      idle = false;
      running = false;
      error = false;
      String change = "Changed from idle to disabled at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
      lcd.clear();
    }
  }
  else if(running){
    //read water level
    waterLevel = adc_read(0);

    //read temperature
    int readDHT = DHT.read11(DHTPIN);
    temp = DHT.temperature;

    //turn on fan
    //digitalWrite(A1, HIGH);
    *port_f |= 0x02;
    //digitalWrite(A2, LOW);
    *port_f &= 0xFB;
    //digitalWrite(A3, HIGH);
    *port_f |= 0x08;

    //digitalWrite(6, LOW); //red LED off
    //digitalWrite(7, LOW); //yellow LED off
    //digitalWrite(8, LOW); //green LED off
    //digitalWrite(9, HIGH); //blue LED on
    setRed(false);
    setYellow(false);
    setGreen(false);
    setBlue(true);

    //running to idle
    if(temp <= temperatureThreshold){
      disabled = false;
      idle = true;
      running = false;
      error = false;
      //digitalWrite(A1, LOW);
      *port_f &= 0xFD;
      String change = "Changed from running to idle at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
      displayDHT();
    }

    //running to error check
    if(waterLevel <= waterThreshold){
      disabled = false;
      idle = false;
      running = false;
      error = true;
      //digitalWrite(A1, LOW);
      *port_f &= 0xFD;
      String change = "Changed from running to error at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
    }

    //running to disabled check
    if(*pin_a & 0x10){
      disabled = true;
      idle = false;
      running = false;
      error = false;
      //digitalWrite(A1, LOW);
      *port_f &= 0xFD;
      String change = "Changed from running to disabled at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
      lcd.clear();
    }    
  }
  else if(error){
    //digitalWrite(6, HIGH); //red LED on
    //digitalWrite(7, LOW); //yellow LED off
    //digitalWrite(8, LOW); //green LED off
    //digitalWrite(9, LOW); //blue LED off
    setRed(true);
    setYellow(false);
    setGreen(false);
    setBlue(false);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Water level");
    lcd.setCursor(0, 1);
    lcd.print("is too low");


    //error to idle check
    if(*pin_a & 0x40){
      disabled = false;
      idle = true;
      running = false;
      error = false;
      String change = "Changed from error to idle at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
      displayDHT();
    }

    //error to disabled check
    if(*pin_a & 0x10){
      disabled = true;
      idle = false;
      running = false;
      error = false;
      String change = "Changed from error to disabled at ";
      for(int i = 0; i < change.length(); i++)
        U0putchar(change[i]);
      displaySerial();
      lcd.clear();
    }
  }

  if(!error){
    if(*pin_a & 0x02) {
      stepper.step(50);
      if(!ventInUse){
        String change = "Vent Started Rotating Counterclockwise at ";
        for(int i = 0; i < change.length(); i++)
          U0putchar(change[i]);
        displaySerial();
        ventInUse = true;
      }
    }
    else if(*pin_a & 0x04){
      if(!ventInUse){
        String change = "Vent Started Rotating Clockwise at ";
        for(int i = 0; i < change.length(); i++)
          U0putchar(change[i]);
        displaySerial();
        ventInUse = true;
      }
    }
    else{
      stepper.step(0);
      ventInUse = false;
    }
  }
  if(!disabled){
    if(now.second() == 0){
      displayDHT();
    }
  }
}

void setBlue(bool on){
  if(on){
    *port_h |= 0x40;
  }
  else{
    *port_h &= 0xBF;
  }
}

void setYellow(bool on){
  if(on){
    *port_h |= 0x10;
  }
  else{
    *port_h &= 0xEF;
  }
}

void setGreen(bool on){
  if(on){
    *port_h |= 0x20;
  }
  else{
    *port_h &= 0xDF;
  }
}

void setRed(bool on){
  if(on){
    *port_h |= 0x08;
  }
  else{
    *port_h &= 0xF7;
  }
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


void startButton() {
  if(disabled){
    disabled = false;
    idle = true;
    running = false;
    error = false;
    String change = "Changed from disabled to idle at ";
    for(int i = 0; i < change.length(); i++)
      U0putchar(change[i]);
    displaySerial();
    displayDHT();
  }
}

void displayDHT()
{
  lcd.clear();
  lcd.setCursor(0,0);
  int readDHT = DHT.read11(DHTPIN);
  temp = DHT.temperature;
  humid = DHT.humidity;
  String tempDisplay = "Temp: " + (String)temp;
  String humidDisplay = "Humidity: " + (String)humid;
  lcd.print(tempDisplay);
  lcd.setCursor(0,1);
  lcd.print(humidDisplay);
}

void displaySerial()
{
  //DateTime now = rtc.now();
  //date
  String output = "";

  output += (String) now.day();
  output += "/";
  output += (String) now.month();
  output += "/";
  output += (String) now.year();
  output += " ";

  //time
  output += (String) now.hour();
  output += ":";
  if(now.minute() < 10)
    output += "0";
  output += now.minute();
  output += ":";
  if(now.second() < 10)
    output += "0";
  output += (String) now.second();
  output += "\n";
  for(int i = 0; i < output.length(); i++)
    U0putchar(output[i]);
}

//converts value to string to shorten decimal place to ensure the value fits on lcd screen
String formatFloat(float value, int length)
{
  String shorter = String(value, length);
  return shorter;
}


