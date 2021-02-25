#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <AS5600.h>

#include <Button2.h>

#include <FastMap.h>

Button2 button35(35);

FastMap upper, lower;

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

AMS_5600 ams5600;

int ang, lang = 0;

void button35_click(Button2 &btn)
{
  Serial.printf("button 35 click\n");
  ams5600.setStartPosition();
}

void button35_doubleClick(Button2 &btn)
{
  Serial.printf("button 35 doubleclick\n");
  // ams5600.setEndPosition();
}

void i2cScanner()
{
  Serial.printf("\n\ni2c scanner\n\n");

  bool found = false;
  elapsedMillis since_looking;

  while (!found)
  {
    Serial.printf("scanning\n");
    for (int addr = 1; addr < 127; addr++)
    {
      Wire.beginTransmission(addr);
      byte error = Wire.endTransmission();

      if (error == 0)
      {
        Serial.printf("device found at 0x02%x\n", addr);
        found = true;
      }
      else if (error == 4)
      {
        Serial.printf("unknown error found at 0x02%x\n", addr);
      }
    }

    if (!found)
      delay(2000);
  }

  Serial.printf("Finished scanning for devices\n");
}

//---------------------------------------------------------

elapsedMillis since_read_angle;
bool outlier = false;
float last_angle = -1,
      mapped = 0,
      mapped_centre = 0,
      mapped_max = 0,
      mapped_min = 0,
      mapped_offset = 0;

#include <MagThrottle.h>

MagThrottle mag_throttle;

void setup()
{
  Serial.begin(115200);
  Serial.printf("Mag Encoder Ready\n");

  button35.setPressedHandler(button35_click);
  button35.setDoubleClickHandler(button35_doubleClick);

  Wire.begin();

  // i2cScanner();

  if (ams5600.detectMagnet() == 0)
  {
    Serial.printf("searching....\n");
    while (1)
    {
      if (ams5600.detectMagnet() == 1)
      {
        Serial.printf("Current Magnitude: %d\n", ams5600.getMagnitude());
        break;
      }
      else
      {
        Serial.println("Can not detect magnet");
      }
      delay(1000);
    }
  }

  mag_throttle.init(&ams5600, &fsm, /*sweep angle*/ 30);

  delay(1000);
}

#include <stdlib.h>
#include <math.h>

uint8_t old_throttle;

void loop()
{
  button35.loop();

  if (since_read_angle > 100)
  {
    since_read_angle = 0;

    uint8_t t = mag_throttle.get();

    old_throttle = t;
  }

  mag_throttle.loop();

  vTaskDelay(10);
}