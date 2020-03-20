

// #ifndef ADAFRUIT_NEOPIXEL_H
// #include <Adafruit_NeoPixel.h>
// #endif

// #define PIXEL_PIN 25

// Adafruit_NeoPixel pixel(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// // #define FLASHER_MID_INTERVAL_MS 2000
// // #define FLASHER_OFF_INTERVAL_MS 100
// // #define FLASHER_BETWEEN_FLASHES_MS 200

// #include <PixelFlasherLib.h>

// PixelFlasherLib status1(&pixel, PIXEL_PIN);

// #define DIM_COLOUR 80

// uint32_t COLOUR_OFF = pixel.Color(0, 0, 0);
// uint32_t COLOUR_RED = pixel.Color(255, 0, 0);
// uint32_t COLOUR_RED_DIM = pixel.Color(DIM_COLOUR, 0, 0);
// uint32_t COLOUR_GREEN = pixel.Color(0, 255, 0);
// uint32_t COLOUR_GREEN_DIM = pixel.Color(0, DIM_COLOUR, 0);
// uint32_t COLOUR_BLUE = pixel.Color(0, 0, 255);
// uint32_t COLOUR_BLUE_DIM = pixel.Color(0, 0, DIM_COLOUR);
// uint32_t COLOUR_WHITE = pixel.Color(255, 255, 255);

// //------------------------------------

// enum PixelEvent
// {
//   PIXEL_NO_EVENT,
//   PIXEL_THROTTLE_MAX,
//   PIXEL_ACCEL,
//   PIXEL_THROTTLE_MIN,
//   PIXEL_BRAKING,
//   PIXEL_THROTTLE_IDLE,
// };

// uint8_t currentEvState = PIXEL_THROTTLE_IDLE;

// //------------------------------------

// const char *getPixelEventName(PixelEvent ev)
// {
//   switch (ev)
//   {
//   case PIXEL_NO_EVENT:
//     return "PIXEL_NO_EVENT";
//   case PIXEL_THROTTLE_MAX:
//     return "PIXEL_THROTTLE_MAX";
//   case PIXEL_THROTTLE_MIN:
//     return "PIXEL_THROTTLE_MIN";
//   case PIXEL_THROTTLE_IDLE:
//     return "PIXEL_THROTTLE_IDLE";
//   case PIXEL_ACCEL:
//     return "PIXEL_ACCEL";
//   case PIXEL_BRAKING:
//     return "PIXEL_BRAKING";
//   default:
//     char buff[20];
//     sprintf(buff, "unhandled ev: %d", (uint8_t)ev);
//     return buff;
//   }
// }
// //------------------------------------

// PixelEvent readFromPixelEventQueue(TickType_t ticks = 5)
// {
//   uint8_t ev;
//   if (xDisplayChangeEventQueue != NULL && xQueueReceive(xDisplayChangeEventQueue, &ev, ticks) == pdPASS)
//   {
//     if (currentEvState != ev)
//     {
// #ifdef PRINT_DISP_STATE_EVENT
//       Serial.printf("<- RX: %s\n", getPixelEventName((PixelEvent)ev));
// #endif
//       currentEvState = ev;
//       return (PixelEvent)ev;
//     }
//   }
//   return PIXEL_NO_EVENT;
// }
// //------------------------------------

// void sendToPixelEventQueue(PixelEvent ev, TickType_t ticks = 10)
// {
// #ifdef PRINT_DISP_STATE_EVENT
//   // Serial.printf("-> PIXEL EVENT: %s\n", getPixelEventName((PixelEvent)ev));
// #endif
//   uint8_t e = (uint8_t)ev;
//   xQueueSendToBack(xDisplayChangeEventQueue, &e, ticks);
// }
// //------------------------------------

// void flasher_task_0(void *pvParameters)
// {
//   pixel.begin();
//   pixel.setPixelColor(0, COLOUR_OFF);
//   pixel.setBrightness(10);
//   pixel.show();

//   status1.init(COLOUR_GREEN);

//   Serial.printf("flasher_task_0 running on core %d\n", xPortGetCoreID());

//   elapsedMillis since_changed_config;

//   while (true)
//   {
//     status1.loop();

//     PixelEvent ev = readFromPixelEventQueue();
//     if (ev != PIXEL_NO_EVENT)
//     {
//       switch (ev)
//       {
//       case PIXEL_THROTTLE_MAX:
//         status1.setColour(COLOUR_BLUE);
//         break;
//       case PIXEL_ACCEL:
//         status1.setColour(COLOUR_BLUE_DIM);
//         break;
//       case PIXEL_THROTTLE_IDLE:
//         status1.setColour(COLOUR_WHITE);
//         break;
//       case PIXEL_BRAKING:
//         status1.setColour(COLOUR_RED_DIM);
//         break;
//       case PIXEL_THROTTLE_MIN:
//         status1.setColour(COLOUR_RED);
//         break;
//       }
//     }
//     vTaskDelay(10);
//   }
//   vTaskDelete(NULL);
// }
// //------------------------------------------------------------
