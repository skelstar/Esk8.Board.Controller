#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <EncoderThrottleLib.h>

//------------------------------------------------------------

EncoderThrottleLib encoder;

//------------------------------------------------------------

void encoder_changed(i2cEncoderLibV2 *obj)
{
  DEBUG("Changed: ");
}

void encoder_push(i2cEncoderLibV2 *obj)
{
  DEBUG("Encoder is pushed!");
}

void encoder_double_push(i2cEncoderLibV2 *obj)
{
  Serial.println("Encoder is double pushed!");
}

//------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);
  Serial.println("**** I2C Encoder V2 basic example ****");

  Wire.begin();
  encoder.init(/*changed*/ encoder_changed,
               /*pushed*/ encoder_push,
               /*double*/ encoder_double_push,
               /*deadman*/ NULL,
               /*min*/ -10,
               /*max*/ 10);
  encoder.setMap(GENTLE);
}

elapsedMillis since_checked_encoder;

void loop()
{
  if (since_checked_encoder > 200)
  {
    since_checked_encoder = 0;
    /* Check the status of the encoder and call the callback */
    encoder.loop();
  }
}