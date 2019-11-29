#ifndef IoTT_Mux64Buttons_h
#define IoTT_Mux64Buttons_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IoTT_ButtonTypeDef.h>
#include <mux64.h>

#define avgBase 5  //numer of values to be considered for rolling average
#define touchMultiplier 200  //multiplier to bring touch values in range of analog vlues

#define noTouchMax 300
#define touchDownMin 1300
#define touchDownMax 1700
#define touchReleaseMin 2700
#define touchReleaseMax 3500
#define noAnalogMin 1000
#define noAnalogMax 3000

#define digitalLoMax 5
#define digitalHiMin 4090

#define btnTypeOff		0x00
#define btnTypeTouch	0x01
#define btnTypeDigital	0x02
#define btnTypeAnalog	0x40
#define btnAutoDetect	0x80

#define newAnalogThreshold 2  //new analog value gets sent out if deviation is more than x%
#define analogMaxVal 4095
/* 
 * supported button types:
 * 0: auto detect (no analog support, digital or touch messages only)
 * 1: digital ButtonN0
 * 2: digital ButtonNC
 * 3: Touch Input
 * 4: Piezo Input
 * 9: Analog Input
 * 
 * Messages for digital buttons: Button down, Button up, Button Click, Button Hold, Button Double Click
 * Messages for Touch and Piezo: same as above, but including analog value
 * Messages Analog Input: Value Change with new value
*/ 

typedef struct 
{
//set from outside
  uint8_t  btnTypeRequested = btnAutoDetect; //auto detect	
  uint16_t btnAddr = 0xFFFF; //LocoNet Button Address
//internal usage
  uint8_t  btnTypeDetected = (btnAutoDetect | btnTypeAnalog | btnTypeDigital | btnTypeTouch); //auto detect	
  uint16_t touchDataBuf = 200;  //latest reading with scaling applied
  uint16_t analogDataBuf = 2000;  //latest reading with scaling applied
  float    touchAvg = 200.0; //rolling average of touchDataBuf
  float    analogAvg = 2000.0; //rolling average of analogDataBuf
  float    touchDiff = 0.0; //rolling average of differential
  float    analogDiff = 0.0; //rolling average of differential
//  float    touchSigma = 100.0; //rolling average of standard deviation
  float    analogSigma = 100.0; //rolling average of standard deviation
  
  uint16_t lastPublishedData = 0;
  uint8_t  detCtr = 100;
  bool     btnStatus = false; //true if pressed
  uint32_t lastStateChgTime[4]; //used to calculate dbl click events
  uint8_t  lastEvtPtr = 0; //bufferPtr to the above

  uint32_t nextHoldUpdateTime; //timer for repeating hold events
  uint32_t analogRefreshTime; //timer for automated analog refreshs
} IoTT_ButtonConfig;

class IoTT_Mux64Buttons
{
	public:
		IoTT_Mux64Buttons();
		~IoTT_Mux64Buttons();

	private:
		IoTT_ButtonConfig * touchArray = NULL;
		Mux muxChip;
		SemaphoreHandle_t buttonBaton;
		uint8_t numTouchButtons;
		uint8_t numRead = 2; //number of reads of buttons in burst read
		uint16_t dblClickThreshold = 1000;
		uint16_t holdThreshold = 500;
		uint16_t analogMinMsgDelay = 750; //minimum time between 2 analog messages. This limits the bandwidth usage
		uint16_t boardBaseAddress = 0;
		int16_t startUpCtr = 50;
		uint16_t analogRefreshInterval = 30000;
		float lpDiv = ((float)avgBase - 1) / (float)avgBase;
		float lpMult = 1 / (float)avgBase;

	public: 
		void initButtons(uint8_t Addr0, uint8_t Addr1, uint8_t Addr2, uint8_t Addr3, uint8_t * analogPins, bool useWifi = false, uint8_t enablePin = -1);
		void loadButtonCfgJSON(DynamicJsonDocument doc);
		void setButtonMode(uint8_t btnNr, uint8_t btnMode, uint16_t btnAddress);
		void setDblClickRate(int dblClickRate);
		void setHoldDelay(int holdDelay);
		void setBoardBaseAddr(int boardAddr);
		void setAnalogRefreshTime(uint16_t newInterval);
		uint8_t  getButtonMode(int btnNr);
		uint16_t  getButtonAddress(int btnNr);
		bool getButtonState(int btnNr);
		void processButtons();
	private:
		void processDigitalButton(uint8_t btnNr, bool btnPressed);
		void processDigitalHold(uint8_t btnNr);
		void sendButtonEvent(uint16_t btnAddr, buttonEvent btnEvent);
		void sendAnalogData(uint8_t btnNr, uint16_t analogValue );
};

extern void onButtonEvent(uint16_t btnAddr, buttonEvent btnEvent) __attribute__ ((weak));
extern void onAnalogData(uint16_t inpAddr, uint16_t analogValue ) __attribute__ ((weak));
extern void onBtnDiagnose(uint8_t evtType, uint8_t portNr, uint16_t inpAddr, uint16_t btnValue) __attribute__ ((weak));

#endif
