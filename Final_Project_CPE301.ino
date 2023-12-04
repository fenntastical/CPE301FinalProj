//Authors: Fenn Edmonds, Liam Francisco, Zack Chotinun, Ashley Eubanks
#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// LCD pins <--> Arduino pins
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

int lowerThreshold = 420;
int upperThreshold = 520;

bool disabled = true; //keeps track of the state of the system
//bool error = false; //keeps track if the system is in ERROR mode

void setup() 
{
  // setup the UART
  U0init(9600);
  // setup the ADC
  adc_init();

  //setting up LED's for water ouput reading
  pinMode(6, OUTPUT); //red LED
  pinMode(7, OUTPUT); //yellow LED
  pinMode(8, OUTPUT); //green LED
  pinMode(9, OUTPUT); //blue LED

  lcd.begin(16, 2); // set up number of columns and rows

 
}
void loop() 
{

  digitalWrite(7, HIGH); //In disabled state the yellow LED should be on

  if(disabled == false)
  {
    digitalWrite(7, LOW); // yellow LED off

    unsigned int input;
    unsigned int input1 = 0;
    unsigned int input2 = 0;
    unsigned int input3 = 0;
    unsigned int input4 = 0;
    input = adc_read(0); //read water level

    if(input >= 1000){
      input1 = input / 1000 + '0';
      input = input % 1000;
    }

    if(input >= 100){
      input2 = input / 100 + '0';
      input = input % 100;
    }

    if(input >= 10){
      input3 = input / 10 + '0';
      input = input % 10;
    }
  
    input4 = input + '0';

    if(input3 < 4 || input3 == 4 && input2 < 2)
    {
      digitalWrite(8, LOW); // green LED off
      digitalWrite(6, HIGH); // red LED on
      //low water lvl (print to screen)
      //ERROR MODE intiated
      lcd.setCursor(0, 0);
      lcd.print("Water level is too low");
    }

    else
    {
      digitalWrite(8, HIGH); // green LED on
      //medium/high water lvl
    }
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
