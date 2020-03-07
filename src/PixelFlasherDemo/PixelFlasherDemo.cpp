#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>
#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN 25

Adafruit_NeoPixel pixel(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// #define FLASHER_MID_INTERVAL_MS 2000
// #define FLASHER_OFF_INTERVAL_MS 100
// #define FLASHER_BETWEEN_FLASHES_MS 200

#include <PixelFlasherLib.h>

PixelFlasherLib status1(&pixel, PIXEL_PIN);

uint32_t COLOUR_OFF = pixel.Color(0, 0, 0);
uint32_t COLOUR_RED = pixel.Color(255, 0, 0);
uint32_t COLOUR_GREEN = pixel.Color(0, 255, 0);
uint32_t COLOUR_BLUE = pixel.Color(0, 0, 255);
uint32_t COLOUR_WHITE = pixel.Color(255, 255, 255);

//------------------------------------

void setup()
{
  Serial.begin(115200);

  uint8_t state = 0;
  uint32_t currentColour = COLOUR_GREEN;

  pixel.begin();
  pixel.setPixelColor(0, currentColour);
  pixel.setBrightness(50);
  pixel.show();

  Serial.printf("flasher_task_1 running on core %d\n", xPortGetCoreID());

  status1.setFlashes(3);
  status1.setColour(currentColour);
}

elapsedMillis since_changed_config;
bool doneChanging1, doneChanging2;

void loop()
{
  while (true)
  {
    status1.loop();

    if (since_changed_config > 20000 && doneChanging2 == false)
    {
      doneChanging2 = true;
      status1.setColour(COLOUR_RED);
      status1.flashOnce(3);
    }
    else if (since_changed_config > 10000 && doneChanging1 == false)
    {
      doneChanging1 = true;
      status1.setColour(COLOUR_BLUE);
      status1.setFlashes(3, /*cycles*/ 2);
    }

    vTaskDelay(10);
  }
}