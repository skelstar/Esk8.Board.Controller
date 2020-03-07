

#ifndef ADAFRUIT_NEOPIXEL_H
#include <Adafruit_NeoPixel.h>
#endif

#ifndef _tasker_h
#include <Tasker.h>
#endif

#define PIXEL_PIN 25

Adafruit_NeoPixel pixel(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOUR_OFF = pixel.Color(0, 0, 0);
uint32_t COLOUR_RED = pixel.Color(255, 0, 0);
uint32_t COLOUR_GREEN = pixel.Color(0, 255, 0);
uint32_t COLOUR_BLUE = pixel.Color(0, 0, 255);
uint32_t COLOUR_WHITE = pixel.Color(255, 255, 255);

Tasker flasher;

#define FLASHER_MID_INTERVAL_MS 2000
#define FLASHER_OFF_INTERVAL_MS 100
#define FLASHER_BETWEEN_FLASHES_MS 500

enum FlashCommand
{
  ON,
  OFF
} currentState;

static uint32_t currentColour;
uint8_t numFlashes = 0;
uint8_t flashCount = 0;

//------------------------------------

//------------------------------------

void setFlashes(uint8_t num)
{
  numFlashes = num;
  flashCount = 0;
}

void setNextState(FlashCommand state)
{
  currentState = state;
}

void flashEvent()
{
  switch (currentState)
  {
  case FlashCommand::ON:
    if (numFlashes == 0)
    {
      return;
    }
    if (flashCount == numFlashes)
    {
      flashCount = 0;
    }
    // DEBUGVAL("ON -> OFF", numFlashes, flashCount);
    pixel.setPixelColor(0, COLOUR_OFF);
    pixel.show();
    setNextState(FlashCommand::OFF);
    flasher.setInterval(flashEvent, FLASHER_OFF_INTERVAL_MS);
    break;
  case FlashCommand::OFF:
    if (flashCount == numFlashes)
    {
      return;
    }
    flashCount++;

    pixel.setPixelColor(0, currentColour);
    pixel.show();
    if (flashCount == numFlashes)
    {
      flasher.setInterval(
          flashEvent,
          flashCount < numFlashes
              ? FLASHER_BETWEEN_FLASHES_MS
              : FLASHER_MID_INTERVAL_MS);
    }
    setNextState(FlashCommand::ON);
    break;
  }
}
//------------------------------------

void flasher_task_1(void *pvParameters)
{
  uint8_t state = 0;
  currentColour = COLOUR_GREEN;

  pixel.begin();
  pixel.setPixelColor(0, currentColour);
  pixel.setBrightness(50);
  pixel.show();

  Serial.printf("flasher_task_1 running on core %d\n", xPortGetCoreID());

  currentState = FlashCommand::ON;
  flasher.setInterval(flashEvent, FLASHER_MID_INTERVAL_MS);

  elapsedMillis since_changed_colour;

  setFlashes(1);
  bool doneChanging1 = false;
  bool doneChanging2 = false;

  while (true)
  {
    flasher.loop();

    if (since_changed_colour > 20000 && doneChanging2 == false)
    {
      doneChanging2 = true;
      setFlashes(0);
    }
    else if (since_changed_colour > 10000 && doneChanging1 == false)
    {
      // since_changed_colour = 0;
      doneChanging1 = true;
      setFlashes(3);
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
