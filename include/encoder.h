#include "i2cEncoderLib.h"

#define USING_ENCODER	1

i2cEncoderLib encoder(0x30); 

int _maxCounts = 0;
int _minCounts = 0;

bool oldCanAccelerate = false;
int _oldCounter = 0;

bool setupEncoder(int maxCounts, int minCounts) {

  _maxCounts = maxCounts;
  _minCounts = minCounts;

	encoder.begin(( INTE_DISABLE | LEDE_DISABLE | WRAP_DISABLE | DIRE_RIGHT | IPUP_DISABLE | RMOD_X1 ));
	encoder.writeCounter(0);
	encoder.writeMax(maxCounts); //Set maximum threshold
	encoder.writeMin(minCounts); //Set minimum threshold
	encoder.writeLEDA(0x00);
	encoder.writeLEDB(0x00);

	Serial.printf("Status = 0x%x \n", encoder.readStatus());

	return encoder.readStatus() != 0xff;
}

bool encoderPressed() {
	return encoder.readStatus(E_PUSH);
}

// map minLimit-0-maxlimit to 0-127-255
int mapCounterTo127to255(int counter)
{
	int rawMiddle = 0;

	if (counter >= rawMiddle)
	{
		return map(counter, rawMiddle, _maxCounts, 127, 255);
	}
	return map(counter, _minCounts, rawMiddle, 0, 127);
}

void handleCounterChanged(int newCounter, bool canAccelerate)
{

	// zero if can't accelerate
	if (newCounter > 0 && canAccelerate == false)
	{
		newCounter = 0;
		encoder.writeCounter(0);
	}

	if (_oldCounter != newCounter)
	{
		// counter has changed
		if (newCounter > 0)
		{
			if (canAccelerate)
			{
				_oldCounter = newCounter;

				int mappedReading = mapCounterTo127to255(_oldCounter);
				encoderChangedEventCallback(mappedReading);
			}
			else
			{
				// can't go above 0
				encoder.writeCounter(0);
			}
		}
		else
		{
			_oldCounter = newCounter;
			int mappedReading = mapCounterTo127to255(_oldCounter);
			encoderChangedEventCallback(mappedReading);
		}
		_oldCounter = newCounter;
	}
}

void encoderUpdate() 
{
	bool newCanAccelerate = getCanAccelerateCallback();
	bool canAccelerateChanged = newCanAccelerate == oldCanAccelerate;

	bool encoderChanged = encoder.updateStatus();

	if (encoderChanged || canAccelerateChanged) {

		int newCounter = encoder.readCounterByte();
		delay(1);

		handleCounterChanged(newCounter, newCanAccelerate);
	
		if (encoder.readStatus(E_PUSH)) {
			encoderPressedEventCallback();
      // Serial.printf("Encoder pressed!\n");
		}
	
		if (encoder.readStatus(E_MAXVALUE)) {
		}
	
		if (encoder.readStatus(E_MINVALUE)) {
		}
	}
}