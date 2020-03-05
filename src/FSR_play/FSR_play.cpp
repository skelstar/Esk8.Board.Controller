#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <FSRThrottleLib.h>

uint8_t brakePin = 34, accelPin = 35;

//------------------------------------------------------------

// note: the _in array should have increasing values
uint8_t multiMap(uint8_t val, uint8_t *_in, uint8_t *_out, uint8_t size)
{
  // take care the value is within range
  if (val <= _in[0])
  {
    return _out[0];
  }
  if (val >= _in[size - 1])
  {
    return _out[size - 1];
  }
  // search right interval
  uint8_t pos = 1; // _in[0] allready tested
  while (val > _in[pos])
  {
    pos++;
  }
  // this will handle all exact "points" in the _in array
  if (val == _in[pos])
  {
    return _out[pos];
  }
  // interpolate in the right segment for the rest
  return (val - _in[pos - 1]) * (_out[pos] - _out[pos - 1]) / (_in[pos] - _in[pos - 1]) + _out[pos - 1];
}

//------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);
  Serial.println("**** I2C Encoder V2 basic example ****");

  pinMode(brakePin, INPUT);
  pinMode(accelPin, INPUT);
}

elapsedMillis since_read_fsr;

const uint16_t brakeMin = 1700, brakeMax = 4095;
const uint16_t accelMin = 1700, accelMax = 4095;

void loop()
{
  if (since_read_fsr > 200)
  {
    since_read_fsr = 0;

    uint8_t brakeIn[] = {0, 30, 70, 127};
    uint8_t brakeOut[] = {0, 50, 90, 127};

    uint16_t brakeRaw = constrain(analogRead(brakePin), brakeMin, brakeMax);
    uint8_t brakeMapped = map(brakeRaw, brakeMax, brakeMin, 127, 0);
    uint8_t brakeMapped2 = multiMap(brakeMapped, brakeIn, brakeOut, sizeof(brakeIn));

    uint8_t accelIn[] = {127, 180, 200, 255};
    uint8_t accelOut[] = {127, 140, 170, 255};

    uint16_t accelRaw = constrain(analogRead(accelPin), accelMin, accelMax);
    uint8_t accelMapped = map(accelRaw, accelMax, accelMin, 127, 255);
    uint8_t accelMapped2 = multiMap(accelMapped, accelIn, accelOut, sizeof(accelIn));

    // DEBUGVAL(brakeMapped, brakeMapped2, accelMapped, accelMapped2);

    const uint8_t graphSize = 20;

    if (brakeMapped2 < 127)
    {
      uint8_t printMapped = map(brakeMapped2, 0, 127, 0, graphSize);
      for (uint8_t i = 0; i <= graphSize; i++)
      {
        Serial.printf("%s", i < printMapped ? "-" : "#");
      }
      Serial.printf("--------------------\n");
    }
    else
    {
      Serial.printf("--------------------");
      uint8_t printMapped = map(accelMapped2, 127, 255, 0, graphSize);
      for (uint8_t i = 0; i <= graphSize; i++)
      {
        Serial.printf("%s", i <= printMapped ? "#" : "-");
      }
      Serial.println();
    }
  }
}