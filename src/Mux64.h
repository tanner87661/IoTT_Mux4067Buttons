#ifndef Mux64_h
#define Mux64_h

#include "Arduino.h"


/* ESP32 characteristics and limitations
 * The ESP32 has 2 12bit ADC's for a total of 18 DA pins
 * ADC1 serves pins 32, 33, 34, 35, 36, 37, 38, 39
 * ADC2 serves pins 0, 2, 4, 12, 13, 14, 15, 25, 26, 27 but not if WiFi is on!!!
 * Touch pins 0, 2, 4, 12, 13, 14, 15, 27, 32, 33
 * Pin	Hex	ADC	Touch	Enc
 * 0	0	2	y		0021
 * 2	2	2	y		0221
 * 4	4	2	y		0421
 * 12	C	2	y		0C21
 * 13	D	2	y		0D21
 * 14	E	2	y		0E21
 * 15	F	2	y		0F21
 * 25	19	2	n		1920
 * 26	1A	2	n		1A20
 * 27	1B	2	y		1B21
 * 32	20	1	y		2011
 * 33	21	1	y		2111
 * 34	22	1	n		2210
 * 35	23	1	n		2310
 * 36	24	1	n		2410
 * 37	25	1	n		2510
 * 38	26	1	n		2610
 * 39	27	1	n		2710
 * 
 */

#define MUX_IO_READ 0
#define MUX_IO_WRITE 1
#define MUX_IO_READ_TOUCH 2

/**
* Mux control lib for CD74HC4067
* 
* Made for / tested with Analog/Digital MUX Breakout from sparkfun: https://www.sparkfun.com/products/9056
*/
class Mux {

   public:
    
	/**
	* Constructor - you'll need to call Mux::setup later if not passing parameters here.
	*/
    Mux();
	
	/**
	* @see Mux::setup
	*/
	Mux(int dPin0, int dPin1, int dPin2, int dPin3, uint8_t * signalPinPtr, bool useWiFi = false, int enablePin=-1);
   
	/**
     *  set Pin # to -1 for not using the pin, i.e. for using only channels 0..3, set dPin2 and dPin3 to -1 and wire the MUX inputs to Vcc
	* setup 
	* @param dPin0		- digital control pin0
	* @param dPin1		- digital control pin1
	* @param dPin2		- digital control pin2
	* @param dPin3		- digital control pin3
	* @param signalPin 	- array of analog io pins to read/write data on. First byte is # of pins to follow
	* @param enablePin 	- optional, use if you have connected the EN (enable) pin to a digtal pin.
	*					 defaults to -1 (unused) if not set here.
	*/
    void setup(int dPin0, int dPin1, int dPin2, int dPin3, uint8_t * signalPinPtr, bool useWiFi = false, int enablePin=-1);
	
	/**
	* setEnabled
	* If the EN (enable) pin is connected to a digital output on the arduino, 
	* and not just connected to ground, use this to enable/disable all IO pins on the board.
	*/
	void setEnabled(bool enabled);
	
	
	/**
	* Read from the chosen channel - the returned value will be in the range of 0 to 1023
	* For reading digital signals, assume a value >= 512 is HIGH and < 512 is LOW - or just use Mux::dRead, which does that mapping for you. Unused pins are ignored, resulting in reading the same input for several addresses.
	* @param channel
	*/
	int readTouch(int channel, int index); //touch read
	
	
	/**
	* @param channel - channel to read from
	* @return HIGH or LOW
	*/
	int readDigital(int channel, int index); //digital read

	int readAnalog(int channel, int index); //analog read
	
	/**
	* @param channel
	* @param value - HIGH or LOW
	**/
    void write(int channel, int index, int value);
    void setWiFiStatus(bool newWiFi);
	
	
  protected:
	uint8_t enablePin; // EN - HIGH==all outputs disabled, LOW==everything is enabled
//	uint8_t * signalPinPtr;
    uint8_t dPin0;
    uint8_t dPin1;
    uint8_t dPin2;
    uint8_t dPin3;
	
	uint8_t  lastIO[4] = {0xFF, 0xFF, 0xFF, 0xFF}; //array with value for each pin whether last operation was read or write. 0xFF means not set (-1)
	uint8_t numPins = 0; //number of analog pins used
	uint8_t signalPin[4] = {0xFF, 0xFF, 0xFF, 0xFF};  
	uint8_t signalCapa[4] = {0x10, 0x10, 0x10, 0x10};  //ADC1, no Touch by default
	int currentChannel;
	bool wifiActive = false;
	
	void setChannel(int channel);
};

#endif
