#define MOTE_ID 3 // 1,2,3,etc.

#include <Time.h>
#include <ADC_7714.h>
#include <SPI.h>
#include <SPISRAM.h>
#include <Streaming.h>

#define ADC_PIN 2 //revision B, C
#define SRAM_PIN 9
#define SRAM_PIN 9
#define SD_PIN 8

ADC_7714 adc(ADC_PIN);
SPISRAM sram(SRAM_PIN);

//semaphore to lock/unlock SPI bus
bool SPI_LOCK = false;

//pointers for SRAM
unsigned int ptr = 0; 
unsigned int start_ptr = 0;
unsigned int end_ptr = 0;

//state variables
boolean sram_filled = false; // this could be initialized to true to avoid writing to SRAM before reset command
boolean high_precision = true; // 24 bit vs 16 bit ADC, defaults to 24 bit

#define SRAM_SIZE 32768
#define SIZE 500
byte buffer[SIZE];

void setup()
{
  Serial.begin(57600);
  delay(100);

  //set SD pin to HIGH to ignore
  pinMode(SD_PIN, OUTPUT);
  digitalWrite(SD_PIN,HIGH);
  
  //setup default SPI for SRAM
  pinMode(SRAM_PIN, OUTPUT);
  pinMode(SRAM_PIN, HIGH);

  //setup ADC
  adc.initialize();
  adc.setChannel(CHANNEL_1_2);
  adc.setGain(GAIN_1);
  adc.setPolarity(BIPOLAR);
  adc.setWordLength((high_precision) ? WORD_LENGTH_24 : WORD_LENGTH_16);
  adc.setFrequency(F10); //F200

  //setup default SPI for SRAM
  pinMode(SRAM_PIN, OUTPUT);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);

  SPI_LOCK = true;
  adc.calibrate();
  adc.synchronize();
  SPI_LOCK = false;

  //ADC uses HW interrupts
  attachInterrupt(1,sense,FALLING);
  
  stop_sensing();
}

void stop_sensing() {
  adc.stop();
}

void start_sensing() {
  adc.start();
  
  ptr = 0;
  start_ptr = 0;
  sram_filled = false;
}

void loop() {
  
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  //if pinged by basestation
  if(Serial.available()){
    while(Serial.available() > 0) {
      char c = Serial.read();
      if(c == 'R'){ // restart sensing
        start_sensing();
        /*delay(MOTE_ID*10-10);
        Serial.print("M");
        Serial.print(MOTE_ID, DEC);
        Serial.print(": ");
        Serial.println("Restarting sensing...");*/
      }
      else if(c == 'S'){ // stop sensing
        stop_sensing();
        /*delay(MOTE_ID*10-10);
        Serial.print("M");
        Serial.print(MOTE_ID, DEC);
        Serial.print(": ");
        Serial.println("Stopped sensing...");*/
      }
      else if(c == 'T'){ // transmit
        delay(10); // wait a bit for byte
        char moteid = Serial.read();
        int int_moteid = moteid - '0';
        if (int_moteid == MOTE_ID) {
          Serial.print("M");
          Serial.print(MOTE_ID, DEC);
          Serial.print(": ");
          Serial.println("Transmitting data...");
          send_data();
        }
      } 
      else if(c == 'C'){ // calibrate
        delay(MOTE_ID*10-10);
        Serial.print("M");
        Serial.print(MOTE_ID, DEC);
        Serial.print(": ");
        Serial.println("Calibrating ADC...");
        detachInterrupt(1);
        SPI_LOCK = true;
        adc.calibrate();
        SPI_LOCK = false;
        attachInterrupt(1,sense,FALLING);
      }
      else if(c == 'F'){ // set ADC sampling frequency
        delay(10); // wait a bit for byte
        char frequency_index = Serial.read();
        int frequency = F10;
        int int_frequency_index = frequency_index - '0';
        delay(MOTE_ID*10-10);
        Serial.print("M");
        Serial.print(MOTE_ID, DEC);
        Serial.print(": ");
        Serial.print("Setting ADC sampling frequency index: ");
        Serial.print(int_frequency_index, DEC);
        Serial.print("\n");
        
        switch (int_frequency_index) {
          case 1: frequency = F10; break;
          case 2: frequency = F100; break;
          case 3: frequency = F200; break;
          case 4: frequency = F250; break;
          case 5: frequency = F500; break;
        }
        //Serial.println(frequency, DEC);
        SPI_LOCK = true;
        adc.setFrequency(frequency);//_index);
        SPI_LOCK = false;
      }
      else if(c == 'G'){ // set ADC gain
        delay(10); // wait a bit for byte
        int gain_log_2 = Serial.read()-'0';
        int gain = GAIN_1;
        delay(MOTE_ID*10-10);
        Serial.print("M");
        Serial.print(MOTE_ID, DEC);
        Serial.print(": ");
        Serial.print("Setting ADC gain_log_2: ");
        Serial.print(gain_log_2, DEC);
        Serial.print("\n");
        
        switch (gain_log_2) {
          case 1: gain = GAIN_1; break;
          case 2: gain = GAIN_2; break;
          case 3: gain = GAIN_4; break;
          case 4: gain = GAIN_8; break;
          case 5: gain = GAIN_16; break;
          case 6: gain = GAIN_32; break;
          case 7: gain = GAIN_64; break;
          case 8: gain = GAIN_128; break;
        }
        //Serial.println(gain, HEX);
        SPI_LOCK = true;
        adc.setGain(gain);
        SPI_LOCK = false;
      }
      else if(c == 'P'){ // set ADC precision
        delay(10); // wait a bit for byte
        char precision = Serial.read();
        
        if (precision == 'H') { // 24-bit precision
          delay(MOTE_ID*10-10);
          Serial.print("M");
          Serial.print(MOTE_ID, DEC);
          Serial.print(": ");
          Serial.print("Setting ADC gain_log_2: ");
          Serial.println("24 bit");
          high_precision = true;
          adc.setWordLength(WORD_LENGTH_24);
        } else if (precision == 'L') { // 16-bit precision
          delay(MOTE_ID*10-10);
          Serial.print("M");
          Serial.print(MOTE_ID, DEC);
          Serial.print(": ");
          Serial.print("Setting ADC gain_log_2: ");
          Serial.println("16 bit");
          high_precision = false;
          adc.setWordLength(WORD_LENGTH_16);
        }
      }
    }
  }
}



void sense()
{
  //check to see if SPI is busy.. if so, ignore sample
  if(SPI_LOCK) {
    return;
  }

  if (!sram_filled) {
    //get conversion and write to SRAM
    SPI_LOCK = true;
    if (high_precision) {
      unsigned long val = adc.getVoltageLong();
      sram.write(ptr, (byte *) &val, sizeof(val));
      SPI_LOCK = false;
      ptr += sizeof(val);
    } else {
      unsigned int val = adc.getVoltageInt();
      sram.write(ptr, (byte *) &val, sizeof(val));
      SPI_LOCK = false;
      ptr += sizeof(val);
    }
    //ptr %= SRAM_SIZE;
    if (ptr > SRAM_SIZE) sram_filled = true;
  }
}

//read and print n elements from SRAM, starting at ptr s
void print_buffer(unsigned int s, int n)
{
  //wait for SPI to clear
  while(SPI_LOCK);

  //read data from SRAM
  SPI_LOCK = true;
  sram.read(s, (byte *) buffer, n);
  SPI_LOCK = false;

  //transmit data
  if (high_precision) {
    for(int j = 0; j < n; j += sizeof(unsigned long)){
      unsigned long *v = (unsigned long *) &buffer[j]; 
      Serial << *v << endl;
    }
  } else {
    for(int j = 0; j < n; j += sizeof(unsigned int)){
      unsigned int *v = (unsigned int *) &buffer[j]; 
      Serial << *v << endl;
    }
  }
}

void send_data()
{
  unsigned int num_elements;

  //assign ending ptr to
  end_ptr = ptr;
  //check for overflow
  if(end_ptr > start_ptr){

    //divide up into N 500 byte element buffers
    while(start_ptr < end_ptr){
      num_elements = end_ptr - start_ptr;
      if(num_elements > SIZE){
        num_elements = SIZE; 
      }
      print_buffer(start_ptr, num_elements);
      start_ptr += num_elements;
    }

  }
  else { // overflow occurs (end_ptr < start_ptr)
    //go up to end of RAM
    while(start_ptr < SRAM_SIZE){
      num_elements = SRAM_SIZE - start_ptr;
      if(num_elements > SIZE){
        num_elements = SIZE; 
      }
      print_buffer(start_ptr, num_elements);
      start_ptr += num_elements;
    }

    //go from 0 to end_ptr
    start_ptr = 0;
    while(start_ptr < end_ptr){
      num_elements = end_ptr - start_ptr;
      if(num_elements > SIZE){
        num_elements = SIZE; 
      }
      print_buffer(start_ptr, num_elements);
      start_ptr += num_elements;
    }
  }
  start_ptr = end_ptr;
}
