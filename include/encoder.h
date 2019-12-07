#include <i2cEncoderLibV2.h>

// https://github.com/Fattoresaimon/ArduinoDuPPaLib/blob/master/examples/I2CEncoderV2/README.md

// i2cEncoderLib encoder(0x30); 	// v1
i2cEncoderLibV2 encoder(0x01); // v2

#define ACCEL_MAX_ENCODER_COUNTS	20
#define BRAKE_MIN_ENCODER_COUNTS	-10

int _oldCounter = 0;

//-------------------------------------------------------

int mapCounterTo127to255(int counter);

//-------------------------------------------------------
int8_t current_counter = 0;

void encoder_handler(i2cEncoderLibV2 *obj) 
{
	int8_t counter = encoder.readCounterByte();
	if (current_counter != counter)
	{
		EventEnum e = EVENT_THROTTLE_CHANGED;
		xQueueSendToFront(xEncoderChangeQueue, &e, pdMS_TO_TICKS(100));
		controller_packet.throttle =  mapCounterTo127to255(counter);
		Serial.printf("Throttle: %d (%d)\n", controller_packet.throttle, counter);
		current_counter = counter;
	}
}

void encoder_deadman_pushed(i2cEncoderLibV2 *obj) 
{
	bool pushed = true;
	xQueueSendToFront(xDeadmanChangedQueue, &pushed, pdMS_TO_TICKS(10));
}

void encoder_deadman_released(i2cEncoderLibV2 *obj) 
{
	bool pushed = false;
	xQueueSendToFront(xDeadmanChangedQueue, &pushed, pdMS_TO_TICKS(10));
}

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
		encoder.writeCounter(0);
		encoder_handler(NULL);
	}
}

void encoder_push(i2cEncoderLibV2 *obj)
{
	// Serial.printf("Encoder is pushed!\n");
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

	// deadman button
	encoder.writeGP1conf(
		i2cEncoderLibV2::GP_IN |
		i2cEncoderLibV2::GP_PULL_EN |
		i2cEncoderLibV2::GP_INT_DI);

	Serial.printf("Max: %d, Min: %d \n", encoder.readMax(), encoder.readMin());

	// Definition of the events
	encoder.onIncrement = encoder_handler;
	encoder.onDecrement = encoder_handler;
	// encoder.onMax = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder max"); };
	// encoder.onMin = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder min"); };
	encoder.onButtonPush = encoder_push;
	// encoder.onButtonRelease = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder release"); };
	// encoder.onButtonDoublePush = [](i2cEncoderLibV2 *obj){ DEBUG("Encoder double push"); };
	encoder.autoconfigInterrupt();

	encoder.onGP1Fall = encoder_deadman_pushed;
	encoder.onGP1Rise = encoder_deadman_released;

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