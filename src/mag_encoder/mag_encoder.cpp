#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <AS5600.h>

#include <Button2.h>

#include <FastMap.h>

#include <utils.h>

Button2 button35(35);

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

AMS_5600 ams5600;

int ang, lang = 0;
//---------------------------------------------------------

#include <MagThrottle.h>

//---------------------------------------------------------

void button35_click(Button2 &btn)
{
  Serial.printf("button 35 click\n");
  MagThrottle::centre(/*print*/ true);
  MagThrottle::fsm.trigger(MagThrottle::TR_CENTRE);
}

//---------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("\n\nMag Encoder Ready\n\n");

  button35.setPressedHandler(button35_click);

  Wire.begin();

  i2cScanner(); // in utils.h

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
        // Serial.printf("   throttle: %d\n", throttle);
      },
      [](uint8_t ev) {
        if (ev == MagThrottle::CLICKED)
          Serial.printf("   event: CLICKED\n");
        else if (ev == MagThrottle::RELEASED)
          Serial.printf("   event: RELEASED\n");
      });

  delay(1000);
}

#include <stdlib.h>
#include <math.h>

uint8_t old_throttle;
elapsedMillis since_read_angle;

void loop()
{
  button35.loop();

  if (since_read_angle > 100)
  {
    since_read_angle = 0;

    // MagThrottle::get();

    // if (MagThrottle::angleChanged())
    MagThrottle::loop();
    // old_throttle = t;
  }

  vTaskDelay(10);
}