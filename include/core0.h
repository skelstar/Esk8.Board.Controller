#include <TFT_eSPI.h>
#include <SPI.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4 // Display backlight control pin
#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

#include <tft.h>

elapsedMillis since_started_tft_setup;

void display_task_1(void *pvParameters)
{
  elapsedMillis since_show_retry_log;

  Serial.printf("display_task_1 running on core %d\n", xPortGetCoreID());

  if (xSPImutex != NULL && xSemaphoreTake(xSPImutex, (TickType_t) 2000) == pdPASS)
  {
    since_started_tft_setup = 0;
    DEBUG("got xSPImutex! (tft)");
    setup_tft();
    DEBUGVAL(since_started_tft_setup);
    vTaskDelay(10);
    xSemaphoreGive(xSPImutex);
  }
  else 
  {
    DEBUG("couldn't get semaphore! (tft)");
  }

  int i = 0;

  while (true)
  {
    if (since_show_retry_log > 2000)
    {
      since_show_retry_log = 0;

      char buff1[20];
      sprintf(buff1, "i: %d", i++);
      tft.drawString(buff1, 20, 20);

      // char buff2[20];
      // // tft.fillScreen(TFT_RED);
      // // line 1
      // // lcd_message(1, buff1, Aligned::ALIGNED_LEFT);
      // // line 2
      // sprintf(buff2, "total: %lu", stats.total_failed);
      // // lcd_message(2, buff2, Aligned::ALIGNED_LEFT);
      // // line 3
      // // lcd_message(3, buff, Aligned::ALIGNED_LEFT);
      // // u8g2.sendBuffer();
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}