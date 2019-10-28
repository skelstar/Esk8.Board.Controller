// #include "i2cEncoderLib.h"
#include <i2cEncoderLibV2.h>

#define USING_ENCODER 1

// i2cEncoderLib encoder(0x30); 	// v1
i2cEncoderLibV2 encoder(0x01); // v2

int32_t _maxCounts = 0;
int32_t _minCounts = 0;

bool oldCanAccelerate = false;
int _oldCounter = 0;

//-------------------------------------------------------

int mapCounterTo127to255(int counter);

//-------------------------------------------------------
void updateMaxCounter(bool to)
{
	if (to == true)
	{
		encoder.writeMax(_maxCounts); //Set maximum threshold
		Serial.printf("Max updated to: %u\n", _maxCounts);
	}
	else
	{
		int prevMax = encoder.readMax();
		encoder.writeMax(0); //Set maximum threshold
		if (prevMax > 0)
		{
			encoder.writeCounter(0);
			Serial.printf("Max updated to: %u (counter: 0)\n", _maxCounts);
		}
	}
}
//-------------------------------------------------------
void encoder_increment(i2cEncoderLibV2 *obj)
{
	int8_t counter = encoder.readCounterByte();

	if (counter < 0)
	{
		int8_t counter = encoder.readCounterByte();
		Serial.printf("Throttle: %d (%d)\n", mapCounterTo127to255(counter), counter);
	}
	else if (counter >= 0 && canAccelerate)
	{
		int8_t counter = encoder.readCounterByte();
		Serial.printf("Throttle: %d (%d)\n", mapCounterTo127to255(counter), counter);
	}
	else
	{
		updateMaxCounter(/*to*/ false);
		int8_t counter = encoder.readCounterByte();
		Serial.printf("Throttle: %d (%d)\n", mapCounterTo127to255(counter), counter);
	}
}

void encoder_decrement(i2cEncoderLibV2 *obj)
{
		int8_t counter = encoder.readCounterByte();
		Serial.printf("Throttle: %d (%d)\n", mapCounterTo127to255(counter), counter);
}

void encoder_max(i2cEncoderLibV2 *obj)
{
}

void encoder_min(i2cEncoderLibV2 *obj)
{
}

void encoder_push(i2cEncoderLibV2 *obj)
{
	updateCanAccelerate(!canAccelerate);
	Serial.printf("Encoder is pushed! Can Accel: %d\n", canAccelerate);
	updateMaxCounter(canAccelerate);
}
//-------------------------------------------------------
bool setupEncoder(int32_t maxCounts, int32_t minCounts)
{

	_maxCounts = maxCounts;
	_minCounts = minCounts;

	encoder.reset();
	encoder.begin(i2cEncoderLibV2::INT_DATA |
								i2cEncoderLibV2::WRAP_DISABLE |
								i2cEncoderLibV2::DIRE_RIGHT |
								i2cEncoderLibV2::IPUP_ENABLE |
								i2cEncoderLibV2::RMOD_X1 |
								i2cEncoderLibV2::STD_ENCODER);
	encoder.writeCounter(0);
	encoder.writeMax(_maxCounts); //Set maximum threshold
	encoder.writeMin(_minCounts); //Set minimum threshold
	encoder.writeStep((int32_t)1);
	encoder.writeAntibouncingPeriod(20); /* Set an anti-bouncing of 200ms */
	encoder.writeDoublePushPeriod(50);	 /*Set a period for the double push of 500ms */

	Serial.printf("Max: %d, Min: %d \n", encoder.readMax(), encoder.readMin());

	// Definition of the events
	encoder.onIncrement = encoder_increment;
	encoder.onDecrement = encoder_decrement;
	encoder.onMax = encoder_max;
	encoder.onMin = encoder_min;
	encoder.onButtonPush = encoder_push;
	// encoder.onButtonRelease = encoder_released;
	// encoder.onButtonDoublePush = encoder_double_push;
	encoder.autoconfigInterrupt();

	Serial.printf("Status = 0x%x \n", encoder.readStatus());

	return encoder.readStatus() != 0xff;
}

bool encoderPressed()
{
	return encoder.readStatus(i2cEncoderLibV2::PUSHP);
}

bool encoderDoublePush()
{
	return encoder.readStatus(i2cEncoderLibV2::PUSHD);
}

// map minLimit-0-maxlimit to 0-127-255
int mapCounterTo127to255(int counter)
{
	int rawMiddle = 0;

	if (counter >= rawMiddle)
	{
		return map(counter, rawMiddle, (int)_maxCounts, 127, 255);
	}
	return map(counter, (int)_minCounts, rawMiddle, 0, 127);
}

void encoderUpdate()
{
	encoder.updateStatus();
}