#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <AS5600.h>

#include <FastMap.h>

#include <utils.h>

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

AMS_5600 ams5600;

int ang, lang = 0;
//---------------------------------------------------------

#include <MagThrottle.h>
#include <NintendoButtons.h>

//---------------------------------------------------------

//---------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("\n\nMag Encoder Ready\n\n");

  Wire.begin();

  i2cScanner(); // in utils.h

  ClassicButtons::init();

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

  MagThrottle::init(
      /*sweep*/ 60,
      [](uint8_t throttle) {
        Serial.printf("   throttle: %d\n", throttle);
      });

  delay(1000);
}

uint8_t old_throttle;
elapsedMillis since_read_angle, since_read_classic;

void loop()
{
  if (since_read_angle > 100)
  {
    since_read_angle = 0;

    MagThrottle::loop();
  }

  if (since_read_classic > 100)
  {
    since_read_classic = 0;
    ClassicButtons::loop();
  }

  vTaskDelay(10);
}