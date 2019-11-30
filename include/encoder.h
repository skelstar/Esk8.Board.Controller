#include <i2cEncoderLibV2.h>

// https://github.com/Fattoresaimon/ArduinoDuPPaLib/blob/master/examples/I2CEncoderV2/README.md

// i2cEncoderLib encoder(0x30); 	// v1
i2cEncoderLibV2 encoder(0x01); // v2

#define ACCEL_MAX_ENCODER_COUNTS	20
#define BRAKE_MIN_ENCODER_COUNTS	-10

// int32_t _maxCounts = 0;
// int32_t _minCounts = 0;

int _oldCounter = 0;

//-------------------------------------------------------

int mapCounterTo127to255(int counter);

//-------------------------------------------------------

void updateEncoderMaxCount(bool accelerateable) 
{
	DEBUGVAL("updateEncoderMaxCount()", accelerateable);
	if (accelerateable) 
	{
		encoder.writeMax(ACCEL_MAX_ENCODER_COUNTS); //Set maximum threshold
	}
	else 
	{
		encoder.writeMax(0); 
	}
}

//-------------------------------------------------------

int8_t currentCounter = 0;

void encoder_increment(i2cEncoderLibV2 *obj)
{
	const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
	EventEnum e = EVENT_THROTTLE_CHANGED;

	int8_t counter = encoder.readCounterByte();

	if (currentCounter != counter)
	{
		currentCounter = counter;
		xQueueSendToFront(xEncoderChangeQueue, &e, xTicksToWait);
		Serial.printf("Throttle: %d (%d)\n", mapCounterTo127to255(counter), counter);
	}
}

void encoder_decrement(i2cEncoderLibV2 *obj)
{
	const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
	EventEnum e = EVENT_THROTTLE_CHANGED;

	int8_t counter = encoder.readCounterByte();
	if (counter != currentCounter) {
		currentCounter = counter;
		xQueueSendToFront(xEncoderChangeQueue, &e, xTicksToWait);
	}
}

void encoder_push(i2cEncoderLibV2 *obj)
{
	Serial.printf("Encoder is pushed!\n");
}
//-------------------------------------------------------
bool setupEncoder(int32_t maxCounts, int32_t minCounts)
{
	encoder.reset();
	encoder.begin(i2cEncoderLibV2::INT_DATA |
								i2cEncoderLibV2::WRAP_DISABLE |
								i2cEncoderLibV2::DIRE_RIGHT |
								i2cEncoderLibV2::IPUP_ENABLE |
								i2cEncoderLibV2::RMOD_X1 |
								i2cEncoderLibV2::STD_ENCODER);
	encoder.writeCounter(0);
	encoder.writeMax(maxCounts); //Set maximum threshold
	encoder.writeMin(minCounts); //Set minimum threshold
	encoder.writeStep((int32_t)1);
	encoder.writeAntibouncingPeriod(20); /* Set an anti-bouncing of 200ms */
	encoder.writeDoublePushPeriod(50);	 /*Set a period for the double push of 500ms */

	Serial.printf("Max: %d, Min: %d \n", encoder.readMax(), encoder.readMin());

	// Definition of the events
	encoder.onIncrement = encoder_increment;
	encoder.onDecrement = encoder_decrement;
	// encoder.onMax = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder max"); };
	// encoder.onMin = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder min"); };
	encoder.onButtonPush = encoder_push;
	// encoder.onButtonRelease = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder release"); };
	// encoder.onButtonDoublePush = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder double push"); };
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
		return map(counter, rawMiddle, (int)ACCEL_MAX_ENCODER_COUNTS, 127, 255);
	}
	return map(counter, (int)BRAKE_MIN_ENCODER_COUNTS, rawMiddle, 0, 127);
}

void encoderUpdate()
{
	encoder.updateStatus();
}