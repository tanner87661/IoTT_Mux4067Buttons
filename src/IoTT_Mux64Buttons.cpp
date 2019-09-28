#include <config.h>
#include <IoTT_Mux64Buttons.h>


uint8_t getButtonTypeByName(String typeName)
{
  if (typeName == "off") return btnTypeOff;
  if (typeName == "auto") return btnAutoDetect;
  if (typeName == "autodetect") return btnAutoDetect;
  if (typeName == "digital") return btnTypeDigital;
  if (typeName == "touch") return btnTypeTouch;
  if (typeName == "analog") return btnTypeAnalog;
  return autodetect; //default
}

IoTT_Mux64Buttons::IoTT_Mux64Buttons()
{
	buttonBaton = xSemaphoreCreateMutex();
}

IoTT_Mux64Buttons::~IoTT_Mux64Buttons()
{
    xSemaphoreTake(buttonBaton, portMAX_DELAY);
    xSemaphoreGive(buttonBaton);
    vSemaphoreDelete(buttonBaton);
}

void IoTT_Mux64Buttons::initButtons(uint8_t Addr0, uint8_t Addr1, uint8_t Addr2, uint8_t Addr3, uint8_t * analogPins, bool useWifi, uint8_t enablePin)
{
	touch_pad_init();
//   	activateFuzzy(fuzzy);
	numTouchButtons = 16 * analogPins[0];
//	Serial.println(numTouchButtons);
	touchArray = (IoTT_ButtonConfig*) realloc (touchArray, numTouchButtons * sizeof(IoTT_ButtonConfig));
	for (int i = 0; i < numTouchButtons; i++)
	{
		IoTT_ButtonConfig * myTouch = &touchArray[i];
		myTouch->btnTypeRequested = btnAutoDetect; //auto detect	
		myTouch->btnTypeDetected = (btnAutoDetect | btnTypeAnalog | btnTypeDigital | btnTypeTouch); //auto detect	
		myTouch->btnAddr = boardBaseAddress + i; //LocoNet Button Address

		myTouch->touchDataBuf = 0;  //latest reading with scaling applied
		myTouch->analogDataBuf = 0; //latest reading with scaling applied
		myTouch->touchDiff = 0;  //latest reading with scaling applied
		myTouch->analogDiff = 0; //latest reading with scaling applied
//		myTouch->touchSigma = 0;  //latest reading with scaling applied
		myTouch->analogSigma = 0; //latest reading with scaling applied

//		myTouch->touchLow = touchThreshold - 2;
//		myTouch->touchHigh = 0;

		myTouch->touchAvg = 0.0; //rolling average
		myTouch->analogAvg = 0.0; //rolling average

//		myTouch->btnTouchThreshold = (uint8_t)touchThreshold;
		myTouch->lastPublishedData = 0;
		myTouch->detCtr = 100;
		myTouch->btnStatus = false; //true if pressed
		myTouch->lastStateChgTime[4] = 0; //used to calculate dbl click

		myTouch->nextHoldUpdateTime = millis() + holdThreshold; 
		myTouch->lastEvtPtr = 0;
		myTouch->analogRefreshTime = millis() + analogRefreshInterval;
	}
    muxChip.setup(Addr0, Addr1, Addr2, Addr3, &analogPins[0], useWifi, enablePin); // initialise on setup d0, d1, d2, d3, analog, enable
}

void IoTT_Mux64Buttons::loadButtonCfgJSON(DynamicJsonDocument doc)
{
	if (doc.containsKey("DblClickThreshold"))
		setDblClickRate((int)doc["DblClickThreshold"]);
    if (doc.containsKey("HoldThreshold"))
        setHoldDelay((int)doc["HoldThreshold"]);
    if (doc.containsKey("BoardBaseAddr"))
        setBoardBaseAddr((int)doc["BoardBaseAddr"]);
    if (doc.containsKey("Buttons"))
    {
        JsonArray Buttons = doc["Buttons"];
        uint8_t buttonDefListLen = Buttons.size();
        for (int i=0; i<buttonDefListLen;i++)
        {
			uint8_t thisPort = Buttons[i]["PortNr"];
			if ((thisPort >= 0) && (thisPort < numTouchButtons))
			{
				touchArray[thisPort].btnAddr = Buttons[i]["ButtonAddr"];
				touchArray[thisPort].btnTypeRequested = getButtonTypeByName(Buttons[i]["ButtonType"]);
				touchArray[thisPort].btnTypeDetected = getButtonTypeByName(Buttons[i]["ButtonType"]);
//				Serial.printf("Button %i type %i addr %i \n", thisPort, touchArray[thisPort].btnTypeRequested, touchArray[thisPort].btnAddr);
			}
        } 
    }  
}

void IoTT_Mux64Buttons::setButtonMode(uint8_t btnNr, uint8_t btnMode, uint16_t btnAddress)
{
    IoTT_ButtonConfig * thisTouchData = &touchArray[btnNr];
//	Serial.printf("Old Button %i type %i addr %i \n", btnNr, thisTouchData->btnTypeRequested, thisTouchData->btnAddr);
	switch (btnMode)
	{
		case btnoff:
		  thisTouchData->btnTypeRequested = btnTypeOff; 
		  thisTouchData->btnTypeDetected = btnTypeOff;
		  break;
		case autodetect: 
		  thisTouchData->btnTypeRequested = btnAutoDetect; 
		  thisTouchData->btnTypeDetected = (btnAutoDetect | btnTypeAnalog | btnTypeDigital | btnTypeTouch);
		  break;
		case digitalAct: 
		  thisTouchData->btnTypeRequested = btnTypeDigital; 
		  thisTouchData->btnTypeDetected =  btnTypeDigital;
		  break;
		case touch: 
		  thisTouchData->btnTypeRequested = btnTypeTouch; 
		  thisTouchData->btnTypeDetected =  btnTypeTouch;
		  break;
		case analog:
		  thisTouchData->btnTypeRequested = btnTypeAnalog; 
		  thisTouchData->btnTypeDetected =  btnTypeAnalog;
		  break;
	}
	thisTouchData->btnAddr = btnAddress;
//	Serial.printf("New Button %i type %i addr %i \n", btnNr, thisTouchData->btnTypeRequested, thisTouchData->btnAddr);
}

void IoTT_Mux64Buttons::setDblClickRate(int dblClickRate)
{
	dblClickThreshold = dblClickRate;
}

void IoTT_Mux64Buttons::setHoldDelay(int holdDelay)
{
//	Serial.printf("Set Hold Delay to %i\n", holdDelay);
	holdThreshold = holdDelay;
}

void IoTT_Mux64Buttons::setBoardBaseAddr(int boardAddr)
{
	boardBaseAddress = boardAddr;
	for (int i = 0; i < numTouchButtons; i++)
	{
		IoTT_ButtonConfig * myTouch = &touchArray[i];
		myTouch->btnAddr = boardBaseAddress + i; //set address for all buttons if boardBase changes
	}
}

void IoTT_Mux64Buttons::setAnalogRefreshTime(uint16_t newInterval)
{
	analogRefreshInterval = newInterval;
}

uint8_t  IoTT_Mux64Buttons::getButtonMode(int btnNr)
{
	return touchArray[btnNr].btnTypeRequested;
}

uint16_t  IoTT_Mux64Buttons::getButtonAddress(int btnNr)
{
	return touchArray[btnNr].btnAddr;
}

bool IoTT_Mux64Buttons::getButtonState(int btnNr)
{
	return &touchArray[btnNr].btnStatus;
}

void IoTT_Mux64Buttons::sendButtonEvent(uint16_t btnAddr, buttonEvent btnEvent)
{
  if ((onButtonEvent) && (startUpCtr==0)) onButtonEvent(btnAddr, btnEvent);
}

void IoTT_Mux64Buttons::sendAnalogData(uint8_t btnNr, uint16_t analogValue)
{
  IoTT_ButtonConfig * thisTouchData = &touchArray[btnNr];
  if ((abs(thisTouchData->lastPublishedData - analogValue) > (newAnalogThreshold * analogMaxVal / 100)) || (((analogValue == 0) || (analogValue == analogMaxVal)) && (analogValue != thisTouchData->lastPublishedData)))
  {
	  if (millis() > thisTouchData->nextHoldUpdateTime)
	  {
		  thisTouchData->lastPublishedData = analogValue;
		  if ((onAnalogData) && (startUpCtr==0)) 
			onAnalogData(thisTouchData->btnAddr, analogValue);
		  thisTouchData->nextHoldUpdateTime = millis() + analogMinMsgDelay;
	  }
  }
}

void IoTT_Mux64Buttons::processDigitalHold(uint8_t btnNr) //call onButtonHold function every holdThreshold milliseconds
{
  IoTT_ButtonConfig * thisTouchData = &touchArray[btnNr];
  if ((thisTouchData->btnStatus) && (millis() > thisTouchData->nextHoldUpdateTime))
  {
	  sendButtonEvent(thisTouchData->btnAddr, onbtnhold);
	  thisTouchData->nextHoldUpdateTime += holdThreshold;
  }
}

void IoTT_Mux64Buttons::processDigitalButton(uint8_t btnNr, bool btnPressed)
{
//	if (btnNr == 2) Serial.println(btnPressed);
  IoTT_ButtonConfig * thisTouchData = &touchArray[btnNr];
  if (btnPressed != thisTouchData->btnStatus)
  {
	thisTouchData->lastEvtPtr++;
	thisTouchData->lastEvtPtr &= 0x03;
	thisTouchData->lastStateChgTime[thisTouchData->lastEvtPtr] = millis();
	thisTouchData->btnStatus = btnPressed;
	if (btnPressed)
	{
		sendButtonEvent(thisTouchData->btnAddr, onbtndown);
		thisTouchData->nextHoldUpdateTime = millis() + holdThreshold;
	}
	else
	{
		sendButtonEvent(thisTouchData->btnAddr, onbtnup);
		//more processing for click and dblclick
		uint8_t prevUpTime = (thisTouchData->lastEvtPtr + 2) &0x03; //same as mod 4
		if ((thisTouchData->lastStateChgTime[prevUpTime] != 0) && ((thisTouchData->lastStateChgTime[thisTouchData->lastEvtPtr] - thisTouchData->lastStateChgTime[prevUpTime]) < dblClickThreshold))
		{
			sendButtonEvent(thisTouchData->btnAddr, onbtndblclick);
		}
		else
		{
			sendButtonEvent(thisTouchData->btnAddr, onbtnclick);
		}
    }
  }
  else
    if (btnPressed)
		processDigitalHold(btnNr);
} 

void IoTT_Mux64Buttons::processButtons()
{
  byte hlpVal;
  startUpCtr = max(startUpCtr-1,0);
  IoTT_ButtonConfig * thisTouchData;
  uint16_t hlpTouch;
  uint16_t hlpAnalog;
  for (uint8_t btnCtr = 0; btnCtr < numTouchButtons; btnCtr++)
  {
	thisTouchData = &touchArray[btnCtr];
    bool useTouch = ((startUpCtr > 0) || ((thisTouchData->btnTypeDetected & btnTypeTouch) > 0));
    bool useAnalog = ((startUpCtr > 0) || ((thisTouchData->btnTypeDetected & (btnTypeDigital | btnTypeAnalog)) > 0));
    int lineNr = btnCtr & 0x0F; //btnCtr % 16;
    int portNr = btnCtr >> 4; //trunc(btnCtr/16);
    for (uint8_t i = 0; i < numRead; i++)
    {
		
// think about eliminating reading and calculating button types that are not used
//	  if (useTouch)
	  {	
	  //read touch new values	
        hlpTouch = touchMultiplier * muxChip.readTouch(lineNr, portNr);
	  //calculate the rolling derivation to previous value
//      thisTouchData->touchDiff = (thisTouchData->touchDiff * lpDiv) + (float(hlpTouch - thisTouchData->touchDataBuf) * lpMult);
	  //calculate the rolling averages
        thisTouchData->touchAvg = ((thisTouchData->touchAvg * lpDiv) + (float(hlpTouch) * lpMult));
	  //calculate the standard deviation
//      thisTouchData->touchSigma = (thisTouchData->touchSigma * lpDiv) + (sq(thisTouchData->touchAvg - hlpTouch) * lpMult);
	  //store current value for next iteration
        thisTouchData->touchDataBuf = hlpTouch;
	  }

// think about eliminating reading and calculating button types that are not used
//	  if (useAnalog)
	  {
      //read analog values
        hlpAnalog = muxChip.readAnalog(lineNr, portNr);
	  //calculate the rolling derivation to previous value
        thisTouchData->analogDiff = (thisTouchData->analogDiff * lpDiv) + (float(hlpAnalog - thisTouchData->analogDataBuf) * lpMult);
	  //calculate the rolling averages
        thisTouchData->analogAvg = (float(thisTouchData->analogAvg) * lpDiv) + (float(hlpAnalog) * lpMult);
	  //calculate the standard deviation
        thisTouchData->analogSigma = (thisTouchData->analogSigma * lpDiv) + (sq(thisTouchData->analogAvg - hlpAnalog) * lpMult);
	  //store current value for next iteration
        thisTouchData->analogDataBuf = hlpAnalog;
        if (abs(thisTouchData->analogDiff) > 250)
          thisTouchData->analogAvg = hlpAnalog; //ensure quick reaction to button presses
	  }
	  
	  if ((startUpCtr == 0) && ((thisTouchData->btnTypeDetected & btnAutoDetect) > 0))
	  {
		  if ((thisTouchData->btnTypeDetected & btnTypeTouch) > 0)
		    if (thisTouchData->touchAvg < noTouchMax)
		      thisTouchData->btnTypeDetected &= ~btnTypeTouch; //clear touch button flag if sensor inactive
		  if ((thisTouchData->btnTypeDetected & btnTypeDigital) > 0)
		    if ((thisTouchData->analogAvg > noAnalogMin) && (thisTouchData->analogAvg < noAnalogMax))
		    {
				thisTouchData->detCtr--;
				if (thisTouchData->detCtr==0)
				{
//		if (btnCtr == 2) 
//					Serial.println(hlpAnalog);
//					Serial.println(thisTouchData->analogAvg);
//		if (btnCtr == 2) 
//					Serial.println("bye digital");
					thisTouchData->btnTypeDetected &= ~btnTypeDigital; //clear digital button flag if analog value detected
				}
		    }
		    else
				thisTouchData->detCtr = 100;
		  if ((thisTouchData->btnTypeDetected & btnTypeAnalog) > 0)
		    if (abs(thisTouchData->analogDiff) > 600)
		    {
//		if (btnCtr == 2) 
//				Serial.println("bye analog");
//				Serial.println(thisTouchData->analogDiff);
		        thisTouchData->btnTypeDetected &= ~btnTypeAnalog; //clear analog button if a digital button was pressed
		    }
	  }
	  if (thisTouchData->btnTypeRequested != btnTypeOff)
	  {
		if (thisTouchData->touchAvg < noTouchMax)
		{
			uint16_t thisAnalogAvg = round(thisTouchData->analogAvg);
			//handle digital or analog button
			if ((thisAnalogAvg >= 0) && (thisAnalogAvg <= analogMaxVal) && ((thisTouchData->btnTypeDetected & btnTypeAnalog) > 0))
				sendAnalogData(btnCtr, thisAnalogAvg);
			if ((thisAnalogAvg >= 0) && (thisAnalogAvg <= digitalLoMax) && ((thisTouchData->btnTypeDetected & btnTypeDigital) > 0))
				processDigitalButton(btnCtr, true);
			if ((thisAnalogAvg >= analogMaxVal) && (thisAnalogAvg >= digitalHiMin) && ((thisTouchData->btnTypeDetected & btnTypeDigital) > 0))
				processDigitalButton(btnCtr, false);
//	    	  if (btnCtr == 2) Serial.println(thisAnalogAvg);
		}
		else
			if ((thisTouchData->touchAvg > touchDownMin) && (thisTouchData->touchAvg < touchDownMax))
				processDigitalButton(btnCtr, true);
			else 
				if ((thisTouchData->touchAvg > touchReleaseMin) && (thisTouchData->touchAvg < touchReleaseMax))
					processDigitalButton(btnCtr, false);
				//else undefined, do nothing
//		if (btnCtr == 2) 
//		Serial.printf("Btn %i %f %f\n", btnCtr, round(thisTouchData->touchAvg), round(thisTouchData->analogAvg));
	  }
	}
  }
  yield();
}
