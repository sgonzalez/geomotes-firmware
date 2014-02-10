#include "Arduino.h"
#include "SPI.h"

//digital IO pins
//#define RDY 9
#define RDY 3

//**Folowing are Specifications for the AD7714
//Comm register
#define BUFFER 0x00
#define COMM_REGISTER 0x00 //0000 0000
#define MODE_REGISTER 0x10 //0001 0000
#define FILTER_HI_REGISTER 0x20 //0010 0000
#define FILTER_LOW_REGISTER 0x30 //
#define TEST_REGISTER 0x40
#define DATA_REGISTER 0x50
#define ZERO_SCALE_REGISTER 0x60
#define FULL_SCALE_REGISTER 0x70
#define READ 0x08 // 0000 1000
#define WRITE 0x00 // 0000 0000

//Channels
#define CHANNEL_1_6 0x00
#define CHANNEL_2_6 0x01
#define CHANNEL_3_6 0x02
#define CHANNEL_4_6 0x03
#define CHANNEL_1_2 0x04
#define CHANNEL_3_4 0x05
#define CHANNEL_5_6 0x06
#define CHANNEL_6_6 0x07 //test mode

//Mode Register
#define NORMAL_MODE 0x00
#define SELF 0x20            //0010 0000
#define ZERO_SCALE_SYS 0x40  //0100 0000
#define FULL_SCALE_SYS 0x60  //0110 0000 
#define SYSTEM_OFFSET 0x80   //1000 0000 
#define BACKGROUND 0xA0      //1010 0000 
#define ZERO_SCALE_SELF 0xC0 //1100 0000 really good?
#define FULL_SCALE_SELF 0xE0 //1110 0000
#define BURNOUT_CURRENT_OFF 0x00
#define BURNOUT_CURRENT_ON 0x02 //default is off
#define FILTER_SYNC_OFF 0x00
#define FILTER_SYNC_ON 0x01 //default is off

//Gain
#define GAIN_1 0x00
#define GAIN_2 0x0    //0000 0100
#define GAIN_4 0x08   //0000 1000
#define GAIN_8 0x0C   //0000 1100
#define GAIN_16 0x10  //0001 0000
#define GAIN_32 0x14  //0001 0100
#define GAIN_64 0x18  //0001 1000
#define GAIN_128 0x1C //0001 1100

//Filter high Register
//Polarity 
#define BIPOLAR 0x00
#define UNIPOLAR 0x80

//_word_Length
#define WORD_LENGTH_16 0x00
#define WORD_LENGTH_24 0x40
#define CURRENT_BOOST_OFF 0x00 //for gains 1 - 4
#define CURRENT_BOOST_ON 0x20  //for gains 8 - 128
#define MASTER_CLK_DISABLE 0x10 //which version do I have??

//frequencies
#define F10 10
#define F100 100
#define F200 200
#define F250 250
#define F500 500

//_hi_filter 
#define DEFAULT_HI_FILTER 0x0F 
#define F10_HI 0x07
#define F100_HI 0x00
#define F200_HI 0x00
#define F250_HI 0x00
#define F500_HI 0x00

//Filter low register
//_low_filter
#define DEFAULT_LOW_FILTER 0xA0
#define F10_LOW 0x08
#define F100_LOW 0xC0
#define F200_LOW 0x60
#define F250_LOW 0x4D
#define F500_LOW 0x26

//SRAM parameters
#define SRAM_BUFFER_SIZE 32768

class ADC_7714
{
public:
    ADC_7714(byte pin);
    
    //start SPI, enable pins, etc.
    void initialize();
    
    //restart data collection
    void synchronize();
    void stop();
    void start();
    
    void calibrate();
    
    //work on this Marc
    void calibrate(byte);
    
    unsigned long getVoltageLong();
    unsigned int getVoltageInt();
    
	void setChannel(byte channel);
    void setGain(byte gain); 
    void setPolarity(byte polarity);
    void setWordLength(byte wordLength);
    void setFrequency(int freq);
    
private:
    int _pin;
	byte _channel;
	byte _gain;
	byte _polarity;
    byte _calibration;
    byte _word_length;
    byte _hi_filter;
    byte _low_filter;
    int _freq;
    
    byte mergeBytes(byte first, byte second, byte third);

    void selectADC();
    void deselectADC();
};
