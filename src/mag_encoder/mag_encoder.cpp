#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <AS5600.h>

#include <FastMap.h>

#include <utils.h>

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

AMS_5600 ams5600;

#include <MagThrottle.h>

#include <NintendoController.h>

#include <NintendoButtons.h>

#include <SparkFun_Qwiic_Button.h>
//https://www.sparkfun.com/products/15932
QwiicButton qwiicButton;

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

  qwiicButton.begin();

  MagThrottle::init(SWEEP_ANGLE, LIMIT_DELTA, MagThrottle::ANTI_CLOCKWISE);
  MagThrottle::setThrottleEnabledCb([] {
    return qwiicButton.isPressed(); // ClassicButtons::buttonPressed(NintendoController::BUTTON_B);
  });

  delay(1000);
}

elapsedMillis since_update_throttle, since_read_classic, since_read_qwiicButton;
uint8_t last_qwiic = 0;

void loop()
{
  if (since_update_throttle > 100)
  {
    since_update_throttle = 0;

    MagThrottle::update();
  }

  if (since_read_qwiicButton > 100)
  {
    since_read_qwiicButton = 0;
    uint8_t pressed = qwiicButton.isPressed();
    if (!pressed && last_qwiic == 1)
    {
      // released
      MagThrottle::centre();
    }
    last_qwiic = pressed;
  }

  if (since_read_classic > 100)
  {
    since_read_classic = 0;
    ClassicButtons::loop();
  }

  vTaskDelay(10);
}