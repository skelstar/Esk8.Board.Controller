#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Library.h>
// #include <SSD1306.h>

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

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

//------------------------------------------------------------------

void nrf_read(uint8_t *data, uint8_t data_len)
{
  nrf24.read_into(data, data_len);
}

void board_packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  DEBUGVAL(board_packet.id);
}

uint8_t send_with_retries(uint8_t *data, uint8_t data_len, uint8_t num_retries)
{
  uint8_t success, retries = 0;
  do
  {
    success = nrf24.sendPacket(00, /*type*/ 0, data, data_len);
    if (success == false)
    {
      vTaskDelay(1);
    }
    // DEBUGVAL(success, retries);
  } while (!success && retries++ < num_retries);

  return retries;
}
//------------------------------------------------------------

SemaphoreHandle_t xCore1Semaphore;

void trigger_read_task_0(void *pvParameters)
{
  elapsedMillis since_read_trigger;
  elapsedMicros since_1;
  uint16_t centre = 0;
  uint8_t old_throttle = 0;

  Serial.printf("\trigger_read_task_0 running on core %d\n", xPortGetCoreID());

#define READ_TRIGGER_PERIOD 100

  while (true)
  {
    if (since_read_trigger > READ_TRIGGER_PERIOD)
    {
      since_read_trigger = 0;

      uint16_t raw;
      // if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)100) == pdTRUE)
      // {
        raw = analogRead(13);
        // xSemaphoreGive(xCore1Semaphore);
      // }
      // else 
      // {
      //   DEBUG("couldn't take semaphore: trigger_read_task_0");
      // }

      if (centre == 0)
      {
        centre = raw;
      }

      controller_packet.throttle = raw > centre
          ? map(raw, centre, 4096, 127, 255)
          : raw < centre
                ? map(raw, 0, centre, 0, 127)
                : 127;
      if (old_throttle != controller_packet.throttle)
      {
        DEBUGVAL(controller_packet.throttle, raw/10);
        old_throttle = controller_packet.throttle;
      }
    }

    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}

//------------------------------------------------------------

uint8_t retries;

#define SEND_TO_BOARD_MS 100

void comms_task_1(void *pvParameters)
{
  Serial.printf("comms_task_1 running on core %d\n", xPortGetCoreID());
  
  elapsedMillis since_sent_to_board, since_requested_response;

  nrf24.begin(&radio, &network, 1, board_packet_available_cb);

  while (true)
  {
    if (since_sent_to_board > SEND_TO_BOARD_MS)
    {
      if (since_sent_to_board > SEND_TO_BOARD_MS + 10)
      {
        DEBUGVAL(since_sent_to_board);
      }
      since_sent_to_board = 0;

      if (since_requested_response > 3000)
      {
        since_requested_response = 0;
        controller_packet.command = 1; // REQUEST
      }
      controller_packet.id++;
      uint8_t bs[sizeof(ControllerData)];
      memcpy(bs, &controller_packet, sizeof(ControllerData));

      // if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)100) == pdTRUE)
      // {
        retries += send_with_retries(bs, sizeof(ControllerData), 5);
        // xSemaphoreGive(xCore1Semaphore);
      // }
      // else 
      // {
      //   DEBUG("couldn't take semaphore: comms_task_1");
      // }

      controller_packet.command = 0;
    }

    nrf24.update();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  Serial.printf("Ready...\n");

  lcd_setup();

  xTaskCreatePinnedToCore(comms_task_1, "comms_task_1", 10000, NULL, /*priority*/ 4, NULL, /*core*/ 1);
  xTaskCreatePinnedToCore(trigger_read_task_0, "trigger_read_task_0", 10000, NULL, /*priority*/ 3, NULL, /*core*/ 0);

  xCore1Semaphore = xSemaphoreCreateMutex();
}

elapsedMillis since_drew_lcd = 0;
bool dot;

void loop()
{
  if (since_drew_lcd > 2000)
  {
    since_drew_lcd = 0;
    char buff[14];
    sprintf(buff, "retries: %d %d", retries, dot);
    dot = !dot;
    lcd_write_text(0, 0, buff, false);
    lcd_write_text(0, 15, buff, false);
    lcd_write_text(0, 30, buff, false);
    lcd_write_text(0, 45, buff, true);
  }
}