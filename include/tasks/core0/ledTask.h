

#if (FEATURE_LED_COUNT > 0)
#include <FastLED.h>
#include "FastLED_RGBW.h"

#define DATA_PIN 27

CRGBW leds[FEATURE_LED_COUNT];
CRGB *ledsRGB = (CRGB *)&leds[0];

#endif

namespace Led
{
  bool ready = false;

  const char *name[] = {"LedTask"};

  BoardClass *myboard;

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Led", xPortGetCoreID());

    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(FEATURE_LED_COUNT));

    for (int i = 0; i < FEATURE_LED_COUNT; i++)
      leds[i] = CRGB::Blue;
    FastLED.show();
    delay(500);
    for (int i = 0; i < FEATURE_LED_COUNT; i++)
      leds[i] = CRGB::Black;
    FastLED.show();

    myboard = new BoardClass();

    ready = true;

    elapsedMillis since_checked_queue;

    while (1)
    {
      if (since_checked_queue > 150)
      {
        since_checked_queue = 0;

        BoardClass *res = boardPacketQueue->peek<BoardClass>();
        if (res != nullptr)
        {
          if (myboard->packet.moving != res->packet.moving)
          {
            for (int i = 0; i < FEATURE_LED_COUNT; i++)
              leds[i] = CRGB::White;
            FastLED.show();
            vTaskDelay(100);
            for (int i = 0; i < FEATURE_LED_COUNT; i++)
              leds[i] = CRGB::Black;
            FastLED.show();
          }
          myboard = new BoardClass(*res);
        }
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //-----------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "LedTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
}