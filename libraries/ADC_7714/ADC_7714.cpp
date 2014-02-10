#include "ADC_7714.h"

ADC_7714::ADC_7714(byte pin) //Keep
{
    // Specifications for AD7714
    // set defaults
    _pin = pin;
    _channel = CHANNEL_1_2;
    _gain = GAIN_1;
    _polarity = BIPOLAR;
    _freq = F10;
    _calibration = NORMAL_MODE;
    _word_length = WORD_LENGTH_24;
    _hi_filter = F10_HI;
    _low_filter =F10_LOW;
}

void ADC_7714::initialize() //keep
{    
    //setup SPI
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.setDataMode(SPI_MODE2);
    
    //setup slave select pin
    pinMode(10, OUTPUT); //required by SPI library
    pinMode(_pin, OUTPUT);
    pinMode(RDY, INPUT);
    
    setChannel(_channel);
    setGain(_gain);
    setPolarity(_polarity);
    setWordLength(_word_length);
    setFrequency(_freq);
    
}

void ADC_7714::setChannel(byte channel)
{
    selectADC();
    
    _channel = channel;
    SPI.transfer(mergeBytes(COMM_REGISTER,READ,_channel));
    SPI.transfer(0x00); //read dummy comm
    
    deselectADC();
}

void ADC_7714::setGain(byte gain)
{
    selectADC();
    
    _gain = gain;
    
    //communicate to COMM register to setup mode, write on channel channel
    SPI.transfer(mergeBytes(MODE_REGISTER,WRITE,_channel));
    
    //write to MODE register to change operating mode (_calibration) and gain
    SPI.transfer(mergeBytes(_calibration, _gain, 0x00));
    deselectADC();    
}

void ADC_7714::setWordLength(byte word_length)
{
    selectADC();
    _word_length = word_length;
    
    SPI.transfer(mergeBytes(WRITE, FILTER_HI_REGISTER, _channel));
    SPI.transfer(mergeBytes(_polarity, _word_length, _hi_filter));
    deselectADC(); 
}

void ADC_7714::setPolarity(byte polarity)
{
    selectADC();
    _polarity = polarity;
    SPI.transfer(mergeBytes(WRITE, FILTER_HI_REGISTER, _channel));
    SPI.transfer(mergeBytes(_polarity, _word_length, _hi_filter));
    deselectADC(); 
}


//add 250 HZ 
void ADC_7714::setFrequency(int freq)
{
    selectADC();
    if(freq == F10){
        _hi_filter = F10_HI;
        _low_filter = F10_LOW;
    } else if(freq == F250){
        _hi_filter = F250_HI;
        _low_filter = F250_LOW;
    } else if(freq == F500){
        _hi_filter = F500_HI;
        _low_filter = F500_LOW;
    } else if(freq == F200){
        _hi_filter = F200_HI;
        _low_filter = F200_LOW;
    } else if(freq == F100){
        _hi_filter = F100_HI;
        _low_filter = F100_LOW;
    }
    
    SPI.transfer(mergeBytes(WRITE, FILTER_HI_REGISTER, _channel));
    SPI.transfer(mergeBytes(_polarity, _word_length, _hi_filter));  
    
    SPI.transfer(mergeBytes(WRITE, FILTER_LOW_REGISTER, _channel));
    SPI.transfer(mergeBytes(_low_filter,0x00,0x00));
        
    
    deselectADC();
}

void ADC_7714::synchronize()
{
    selectADC();

//    SPI.transfer(mergeBytes(MODE_REGISTER,WRITE,_channel));
//    SPI.transfer(mergeBytes(_calibration, _gain, FILTER_SYNC_ON));
//    
//    SPI.transfer(mergeBytes(MODE_REGISTER,WRITE,_channel));
//    SPI.transfer(mergeBytes(_calibration, _gain, FILTER_SYNC_OFF));
    stop();
    start();
    
    deselectADC();
}


/*
. When this bit is high, the nodes of the digital filter, the filter control logic and the calibration control logic are held in a reset state and the analog modulator is also held in its reset
state. When this bit goes low, the modulator and filter start to process data and a valid word is available
*/

void ADC_7714::stop()
{
    selectADC();

    //pull FILTER_SYNC bit high to hold conversions
    SPI.transfer(mergeBytes(MODE_REGISTER,WRITE,_channel));
    SPI.transfer(mergeBytes(_calibration, _gain, FILTER_SYNC_ON));
    
    deselectADC();
}

void ADC_7714::start()
{
    selectADC();
    
    //pull FILTER_SYNC bit low to restart conversion
    SPI.transfer(mergeBytes(MODE_REGISTER,WRITE,_channel));
    SPI.transfer(mergeBytes(_calibration, _gain, FILTER_SYNC_OFF));
    
    deselectADC();
}

unsigned long ADC_7714::getVoltageLong()
{
    selectADC();
    SPI.transfer(0x5c);
    
    byte b[4];
    
    //must fill in the 4th byte with 0
    b[3] = 0x00;
    
    //3 bytes from ADC...
    b[2] = SPI.transfer(0x00);
    b[1] = SPI.transfer(0x00);
    b[0] = SPI.transfer(0x00);
    
    deselectADC();
    
    //point the pointer at the byte array
    unsigned long *ptr = (unsigned long *) b;
    
    // dereference the pointer (with *).. this should be 1
    return *ptr;
}

unsigned int ADC_7714::getVoltageInt()
{
    selectADC();
    SPI.transfer(0x5c);
    
    byte b[2];
    
    b[1] = SPI.transfer(0x00);
    b[0] = SPI.transfer(0x00);
    
    deselectADC();
    
    //point the pointer at the byte array
    unsigned int *ptr = (unsigned int *) b;
    
    // dereference the pointer (with *).. this should be 1
    return *ptr;
}

//performs all calibration steps
void ADC_7714::calibrate()
{   
    calibrate(SELF);
    calibrate(ZERO_SCALE_SYS);
    calibrate(FULL_SCALE_SYS);
    calibrate(SYSTEM_OFFSET);
    calibrate(BACKGROUND);
    calibrate(ZERO_SCALE_SELF);
    calibrate(FULL_SCALE_SELF);
}

//HW interrupts must be turned off for this!!
void ADC_7714::calibrate(byte calibrationType)
{
    selectADC();
    
    //write to mode register
    SPI.transfer(mergeBytes(MODE_REGISTER,WRITE,_channel));
    SPI.transfer(mergeBytes(calibrationType, _gain, 0x00));  
    
    //RDY goes HIGH when calibration starts, goes LOW when finished
    while(digitalRead(RDY));
        
    deselectADC();
}


byte ADC_7714::mergeBytes(byte first, byte second, byte third) {
    byte result = BUFFER | first;
    result = result | second;
    result = result | third;
    return result;
    
}

void ADC_7714::selectADC()
{
    //change SPI mode
    //SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.setDataMode(SPI_MODE2);
    digitalWrite(_pin, LOW);
}

void ADC_7714::deselectADC()
{
    //change SPI mode back to SRAM
    //SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.setDataMode(SPI_MODE0);
    digitalWrite(_pin, HIGH);
}
