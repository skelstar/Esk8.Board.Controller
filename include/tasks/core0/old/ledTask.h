

// #include <FastLED.h>
// #include "FastLED_RGBW.h"
// #ifndef FSMMANAGER_H
// #include <FsmManager.h>
// #endif

// #define DATA_PIN 27

// CRGBW leds[FEATURE_LED_COUNT];
// CRGB *ledsRGB = (CRGB *)&leds[0];

// namespace Led
// {
//   bool ready = false;

//   const char *name[] = {"LedTask"};

//   CRGB fill_col;

//   enum Trigger
//   {
//     IDLE,
//     FLASH,
//   };

//   const char *getTrigger(uint8_t tr)
//   {
//     switch (tr)
//     {
//     case IDLE:
//       return "IDLE";
//     case FLASH:
//       return "FLASH";
//     }
//     return "OUT OF RANGE: getTrigger()";
//   }

//   void ledsFill(CRGB col)
//   {
//     for (int i = 0; i < FEATURE_LED_COUNT; i++)
//       leds[i] = col;
//     FastLED.show();
//   }
//   FsmManager<Trigger> fsm;

//   State stIdle(
//       [] {
//         ledsFill(CRGB::Black);
//       },
//       NULL, NULL);

//   elapsedMillis since_started_flash;

//   State stFlashLed(
//       [] {
//         ledsFill(CRGB::White);
//         since_started_flash = 0;
//       },
//       [] {
//         if (since_started_flash > 200)
//         {
//           ledsFill(CRGB::Black);
//           fsm.trigger(IDLE);
//         }
//       },
//       NULL);

//   Fsm _fsm(&stIdle);

//   void add_transitions()
//   {
//     _fsm.add_transition(&stIdle, &stFlashLed, FLASH, NULL);
//     _fsm.add_transition(&stFlashLed, &stIdle, IDLE, NULL);
//   }

//   BoardClass *myboard;

//   void task(void *pvParameters)
//   {
//     Serial.printf(PRINT_TASK_STARTED_FORMAT, "Led", xPortGetCoreID());

//     FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(FEATURE_LED_COUNT));

//     for (int i = 0; i < FEATURE_LED_COUNT; i++)
//       leds[i] = CRGB::Blue;
//     FastLED.show();
//     delay(500);
//     for (int i = 0; i < FEATURE_LED_COUNT; i++)
//       leds[i] = CRGB::Black;
//     FastLED.show();

//     myboard = new BoardClass();

//     fsm.begin(&_fsm);
//     add_transitions();

//     ready = true;

//     elapsedMillis since_checked_queue;

//     while (1)
//     {
//       if (since_checked_queue > 150)
//       {
//         since_checked_queue = 0;

//         BoardClass *res = boardPacketQueue->peek<BoardClass>();
//         if (res != nullptr)
//         {
//           if (myboard->packet.moving != res->packet.moving)
//           {
//             fsm.trigger(Trigger::FLASH);
//           }
//           myboard = new BoardClass(*res);
//         }
//       }

//       fsm.runMachine();

//       vTaskDelay(10);
//     }
//     vTaskDelete(NULL);
//   }
//   //-----------------------------------------------------

//   void createTask(uint8_t core, uint8_t priority)
//   {
//     xTaskCreatePinnedToCore(
//         task,
//         "LedTask",
//         10000,
//         NULL,
//         priority,
//         NULL,
//         core);
//   }
// }