#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <Smoothed.h>

#ifndef Wire
#include <Wire.h>
#endif

#include <U8g2lib.h>

//------------------------------------------------------------------

#define SPI_CE 33
#define SPI_CS 26

#define OLED_SCL 15
#define OLED_SDA 4
#define OLED_RST 16
#define OLED_ADDR 0x3C
#define OLED_CONTRAST_HIGH 100 // 256 highest
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RST);

//--------------------------------------------------------------------------------
void lcd_setup()
{
  u8g2.begin();
  u8g2.setContrast(OLED_CONTRAST_HIGH);
  u8g2.clearBuffer();
}

#define FONT_SIZE_MED u8g2_font_profont17_tr

/* 172 ms*/
void lcd_write_text(uint8_t x, uint8_t y, char *text, bool send)
{
  u8g2.setFont(FONT_SIZE_MED); // full
  u8g2.setFontPosTop();
  u8g2.setDrawColor(1);
  u8g2.drawStr(x, y, text);
  if (send)
  {
    u8g2.sendBuffer();
  }
}
//------------------------------------------------------------------

VescData old_vescdata, board_packet;

ControllerData controller_packet, old_packet;

//------------------------------------------------------------------

#include "core0.h"

//------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  Serial.printf("Ready...\n");

  lcd_setup();

  xTaskCreatePinnedToCore(comms_task_0, "comms_task_0", 5000, NULL, /*priority*/ 4, NULL, /*core*/ 0);
  xTaskCreatePinnedToCore(trigger_read_task_0, "trigger_read_task_0", 1024, NULL, /*priority*/ 3, NULL, /*core*/ 0);
}

elapsedMillis since_drew_lcd = 0;
bool dot;

void loop()
{
  if (since_drew_lcd > 2000)
  {
    since_drew_lcd = 0;
    char buff[20];
    sprintf(buff, "retries: %d %d", 0, dot);
    dot = !dot;
    u8g2.clearBuffer();
    lcd_write_text(0, 0, buff, false);
    lcd_write_text(0, 15, buff, false);
    lcd_write_text(0, 30, buff, false);
    lcd_write_text(0, 45, buff, true);
  }
}