#include "Arduino.h"
#include "Mux64.h"
#include "WiFi.h"

#define totalPins 18
uint16_t portCapa[totalPins] = {0x0021, 0x0221, 0x0421, 0x0C21, 0x0D21, 0x0E21, 0x0F21, 0x1920, 0x1A20, 0x1B21, 0x2011, 0x2111, 0x2210, 0x2310, 0x2410, 0x2510, 0x2610, 0x2710};


/**
* Construct - no setup
**/
Mux::Mux() {
	//
}


/**
* Construct + setup
**/
Mux::Mux(int dPin0, int dPin1, int dPin2, int dPin3, uint8_t * signalPinPtr, bool useWiFi, int enablePin) {
	setup(dPin0,dPin0,dPin0,dPin0,signalPin,useWiFi, enablePin);
}


/**
* @public
*/
void Mux::setup(int dPin0, int dPin1, int dPin2, int dPin3, uint8_t * signalPinPtr, bool useWiFi, int enablePin){
	
	this->dPin0 			= dPin0;
	this->dPin1 			= dPin1;
	this->dPin2 			= dPin2;
	this->dPin3 			= dPin3;
	this->enablePin			= enablePin;
	currentChannel 			= -1;
	wifiActive 				= useWiFi;
	
	// using the EN pin?
	if(enablePin>-1) 
	{
		pinMode(enablePin, OUTPUT);	
		digitalWrite(enablePin, LOW); // start enabled
	}
	
	int pins[4] = {dPin0,dPin1,dPin2,dPin3};
	
	for(uint8_t i=0;i<4;i++){
		// set pinMode for the digital control pins 
		if (pins[i] >= 0)
		{
			pinMode(pins[i], OUTPUT);
			// set all control pins LOW 
			digitalWrite(pins[i], LOW);
		}
	}
	numPins = *(&signalPinPtr[0]);
	memcpy(signalPin, &signalPinPtr[1], numPins);
	for(uint8_t i=0;i<numPins;i++)
		for(uint8_t j=0;j<totalPins;j++)
		{
			if (signalPin[i] == ((portCapa[j] & 0xFF00) >> 8))
			{
				signalCapa[i] = (portCapa[j] & 0x00FF);
//				Serial.println(signalCapa[i]);
				break;
			}
		}
}


/**
* @public
*/
void Mux::setEnabled(bool enabled){
	if(enablePin!=-1){
		digitalWrite(enablePin, enabled ? LOW : HIGH);
	}
}

	
/**
* @public
*
**/	
int Mux::readTouch(int channel, int index)
{
	if ((signalCapa[index] & 0x01) > 0) //has Touch capability
	{
		if(*(&lastIO[index]) != MUX_IO_READ_TOUCH) 
		{
			pinMode(*(&signalPin[index]), INPUT);
			*(&lastIO[index]) = MUX_IO_READ_TOUCH;
		}
		setChannel(channel);
		return touchRead(*(&signalPin[index]));
	}
	else
		return 0; //reading below noTouchMax as there is not Touch
}


/**
* @public
*
**/
int Mux::readDigital(int channel, int index)
{
	if(*(&lastIO[index]) != MUX_IO_READ) 
	{
		pinMode(*(&signalPin[index]), INPUT_PULLUP);
		*(&lastIO[index]) = MUX_IO_READ;
	}
	setChannel(channel);
	return digitalRead(*(&signalPin[index]));
}

int Mux::readAnalog(int channel, int index){
	if (((signalCapa[index] & 0x00F0) == 0x10) or (not wifiActive)) //is on ADC1 or no Wifi active
	{
		if(*(&lastIO[index]) != MUX_IO_READ) 
		{
			pinMode(*(&signalPin[index]), INPUT_PULLUP);
			*(&lastIO[index]) = MUX_IO_READ;
		}
		setChannel(channel);
		return analogRead(*(&signalPin[index]));
	}
	else
		return (0x03FFF * readDigital(channel, index)); //no analog read capability, so we fake it with digital read, which is always availalbe
}

/**
* @public
* 
**/
void Mux::write(int channel, int index, int value){
	if(*(&lastIO[index]) != MUX_IO_WRITE) 
	{
		pinMode(*(&signalPin[index]), OUTPUT);
		*(&lastIO[index]) = MUX_IO_WRITE;
	}
	setChannel(channel);
	digitalWrite(*(&signalPin[index]), value);
}

void Mux::setWiFiStatus(bool newWiFi)
{
	wifiActive = newWiFi;
}


/**
* @private 
* set the current mux channel [0-15] using 4 digtal pins to write the 4 bit integer
*/
void Mux::setChannel(int channel){
	if(currentChannel != channel) {
//		Serial.printf("Channel: %d \n", channel);
		if (dPin0 >= 0)
			digitalWrite(dPin0, bitRead(channel,0));
		if (dPin1 >= 0)
			digitalWrite(dPin1, bitRead(channel,1));
		if (dPin2 >= 0)
			digitalWrite(dPin2, bitRead(channel,2));
		if (dPin3 >= 0)
			digitalWrite(dPin3, bitRead(channel,3));
		currentChannel = channel;
//		delay(5);
	}
}
