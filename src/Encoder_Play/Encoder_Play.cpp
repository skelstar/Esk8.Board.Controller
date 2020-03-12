#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <i2cEncoderLibV2.h>

//------------------------------------------------------------

i2cEncoderLibV2 encoder(0x01); /* A0 is soldered */

//------------------------------------------------------------

//Callback when the CVAL is incremented
void encoder_increment(i2cEncoderLibV2 *obj)
{
  Serial.print("Increment: ");
  Serial.println(encoder.readCounterByte());
}

//Callback when the CVAL is decremented
void encoder_decrement(i2cEncoderLibV2 *obj)
{
  Serial.print("Decrement: ");
  Serial.println(encoder.readCounterByte());
}

//Callback when CVAL reach MAX
void encoder_max(i2cEncoderLibV2 *obj)
{
  Serial.print("Maximum threshold: ");
  Serial.println(encoder.readCounterByte());
}

//Callback when CVAL reach MIN
void encoder_min(i2cEncoderLibV2 *obj)
{
  Serial.print("Minimum threshold: ");
  Serial.println(encoder.readCounterByte());
}

//Callback when the encoder is pushed
void encoder_push(i2cEncoderLibV2 *obj)
{
  Serial.println("Encoder is pushed!");
}

//Callback when the encoder is released
void encoder_released(i2cEncoderLibV2 *obj)
{
  Serial.println("Encoder is released");
}

//Callback when the encoder is double pushed
void encoder_double_push(i2cEncoderLibV2 *obj)
{
  Serial.println("Encoder is double pushed!");
}

//------------------------------------------------------------

void setup(void)
{
  // pinMode(IntPin, INPUT);
  Wire.begin();
  Serial.begin(115200);
  Serial.println("**** I2C Encoder V2 basic example ****");
  /*
    INT_DATA= The register are considered integer.
    WRAP_DISABLE= The WRAP option is disabled
    DIRE_LEFT= Encoder left direction increase the value
    IPUP_ENABLE= INT pin have the pull-up enabled.
    RMOD_X1= Encoder configured as X1.
    RGB_ENCODER= type of encoder is RGB, change to STD_ENCODER in case you are using a normal rotary encoder.
  */
  encoder.reset();
  encoder.begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE | i2cEncoderLibV2::DIRE_LEFT | i2cEncoderLibV2::IPUP_ENABLE | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
  // Use this in case of standard encoder!
  //  Encoder.begin(i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE | i2cEncoderLibV2::DIRE_LEFT | i2cEncoderLibV2::IPUP_ENABLE | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::STD_ENCODER);

  // try also this!
  //  Encoder.begin(i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_ENABLE | i2cEncoderLibV2::DIRE_LEFT | i2cEncoderLibV2::IPUP_ENABLE | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

  encoder.writeCounter((int32_t)0);    /* Reset the counter value */
  encoder.writeMax((int32_t)10);       /* Set the maximum threshold*/
  encoder.writeMin((int32_t)-10);      /* Set the minimum threshold */
  encoder.writeStep((int32_t)1);       /* Set the step to 1*/
  encoder.writeAntibouncingPeriod(20); /* Set an anti-bouncing of 200ms */
  encoder.writeDoublePushPeriod(50);   /*Set a period for the double push of 500ms */

  // Definition of the events
  encoder.onIncrement = encoder_increment;
  encoder.onDecrement = encoder_decrement;
  encoder.onMax = encoder_max;
  encoder.onMin = encoder_min;
  encoder.onButtonPush = encoder_push;
  encoder.onButtonRelease = encoder_released;
  encoder.onButtonDoublePush = encoder_double_push;

  /* Enable the I2C Encoder V2 interrupts according to the previus attached callback */
  encoder.autoconfigInterrupt();
}

elapsedMillis since_checked_encoder;

void loop()
{
  if (since_checked_encoder > 200)
  {
    since_checked_encoder = 0;
    /* Check the status of the encoder and call the callback */
    encoder.updateStatus();
  }
}