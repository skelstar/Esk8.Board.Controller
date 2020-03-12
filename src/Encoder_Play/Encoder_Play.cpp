#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>
#include <Button2.h>

Button2 button0(0);

#ifndef READ_TRIGGER_PERIOD
#define READ_TRIGGER_PERIOD 200
#endif

#include <EncoderThrottleLib.h>

//------------------------------------------------------------

EncoderThrottleLib encoder;

//------------------------------------------------------------

void encoder_changed(i2cEncoderLibV2 *obj)
{
  // uint8_t throttle = encoder.mapCounterToThrottle(/*print*/ true);
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
               /*min*/ -8,
               /*max*/ 8);
  // encoder.setMap(GENTLE);
}

elapsedMillis since_checked_encoder;

void loop()
{
  if (since_checked_encoder > 200)
  {
    button0.loop();

    since_checked_encoder = 0;
    /* Check the status of the encoder and call the callback */
    encoder.loop(button0.isPressed());
  }
}