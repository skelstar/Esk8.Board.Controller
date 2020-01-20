#ifdef SERIAL__DEBUG
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <SPI.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <Smoothed.h>

// #include <SSD1306.h>

#define SPI_MOSI 23 // blue?
#define SPI_MISO 19 // orange?
#define SPI_CLK 18  // yellow
// #define SPI_CE    33  // white
// #define SPI_CS    26  // green
#define SPI_CS 12 // green
#define SPI_CE 16 // 15 // white

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

VescData board_packet;

ControllerData controller_packet;
ControllerConfig controller_config;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

class Stats
{
public:
  unsigned long total_failed;
} stats;

#define NUM_RETRIES 5
#define SEND_TO_BOARD_INTERVAL 200

elapsedMillis since_sent_to_board;

//------------------------------------------------------------

elapsedMillis since_read_trigger;

Smoothed<float> retry_log;

#define READ_TRIGGER_PERIOD 200
#define SMOOTH_OVER_MILLIS 2000

SemaphoreHandle_t xSPImutex;

//------------------------------------------------------------

#include <comms_2.h>
#include <TriggerLib.h>
#include <peripherals.h>

#include <core0.h>

TriggerLib trigger(/*pin*/ 13, /*deadzone*/ 10);

void read_trigger()
{
  uint8_t old_throttle = controller_packet.throttle;

  controller_packet.throttle = trigger.get_throttle();

#ifdef PRINT_THROTTLE
  if (old_throttle != controller_packet.throttle)
  {
    DEBUGVAL(controller_packet.throttle);
    old_throttle = controller_packet.throttle;
  }
#endif
}

void send_task_1(void *pvParameters)
{
  Serial.printf("------------------\nsend_task_1 running on core %d\n------------------\n", xPortGetCoreID());

  bool done = false;

  if (xSPImutex != NULL && xSemaphoreTake(xSPImutex, (TickType_t)2000) == pdPASS)
  {
    nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);
    xSemaphoreGive(xSPImutex);
    DEBUG("nrf setup!");
    done = true;
  }

  while (true)
  {
    if (since_read_trigger > READ_TRIGGER_PERIOD)
    {
      since_read_trigger = 0;
      read_trigger();
    }

    if (since_sent_to_board > SEND_TO_BOARD_INTERVAL)
    {
      since_sent_to_board = 0;
      send_control_packet_to_board();
    }

    if (xSPImutex != NULL && xSemaphoreTake(xSPImutex, (TickType_t)10) == pdPASS)
    {
      // DEBUG("nrf loop");
      nrf24.update();
      xSemaphoreGive(xSPImutex);
    }
    else
    {
      // DEBUG("couldn't get semaphore (nrf.update()");
    }

    button0.loop();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

#define LOG_LENGTH_MILLIS 5000
  retry_log.begin(SMOOTHED_AVERAGE, LOG_LENGTH_MILLIS / SEND_TO_BOARD_INTERVAL);

  controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
  controller_config.throttle_smoothing_period = 2000; // ignored for now
  send_config_packet_to_board();

  trigger.initialise();

  button_init();

  xSPImutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(send_task_1, "send_task_1", 10000, NULL, /*priority*/ 4, NULL, /*core*/ 1);
  vTaskDelay(2000);
  xTaskCreatePinnedToCore(display_task_1, "display_task_1", 10000, NULL, /*priority*/ 4, NULL, /*core*/ 0);
}

void loop()
{
  vTaskDelete(NULL);
}
//------------------------------------------------------------------
