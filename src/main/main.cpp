#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define NRF_CE 26
#define NRF_CS 33

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <Smoothed.h>

#include <TFT_eSPI.h>

//------------------------------------------------------------------

#define DEADMAN_PIN 17
#define TRIGGER_ANALOG_PIN 27

//------------------------------------------------------------------

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

VescData board_packet;
ControllerData controller_packet;
ControllerConfig controller_config;
//------------------------------------------------------------------

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

#define NUM_RETRIES 5
#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
#endif
//------------------------------------------------------------------

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library
//------------------------------------------------------------------

class Stats
{
public:
  unsigned long total_failed;
  unsigned long num_packets_with_retries;
} stats;

elapsedMillis since_sent_to_board;
elapsedMillis since_read_trigger;

uint16_t remote_battery_percent = 0;
bool throttle_enabled = true;

Smoothed<float> retry_log;

#define SMOOTH_OVER_MILLIS 2000

//------------------------------------------------------------

enum DeadmanEvent
{
  EV_DEADMAN_NO_EVENT,
  EV_DEADMAN_PRESSED,
  EV_DEADMAN_RELEASED,
};
//------------------------------------------------------------

xQueueHandle xDeadmanQueueEvent;
xQueueHandle xDisplayChangeEventQueue;
//------------------------------------------------------------------
#include <TriggerLib.h>
void trigger_changed_cb();

TriggerLib trigger(
    TRIGGER_ANALOG_PIN,
    trigger_changed_cb,
    /*deadzone*/ 10);
//------------------------------------------------------------------

#include <utils.h>
#include <features/deadman.h>
#include <screens.h>
#include <menu_system.h>
#include <comms_connected_state.h>
#include <nrf_comms.h>

#include <display_task_0.h>
#include <features/battery_measure.h>
#include <core1.h>

#include <peripherals.h>

//------------------------------------------------------------------
void trigger_changed_cb()
{
  send_to_display_event_queue(DISP_EV_REFRESH, 5);
}
//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

#define LOG_LENGTH_MILLIS 5000
  retry_log.begin(SMOOTHED_AVERAGE, LOG_LENGTH_MILLIS / SEND_TO_BOARD_INTERVAL);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);

  delay(200);

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  print_build_status();

  controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
  send_config_packet_to_board();

  trigger.initialise();
#ifdef FEATURE_DEADMAN
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  trigger.set_deadman_pin(DEADMAN_PIN);
#endif

  // core 0
#ifdef FEATURE_DEADMAN
  xTaskCreatePinnedToCore(deadmanTask_0, "deadmanTask_0", 4092, NULL, /*priority*/ 4, NULL, 0);
#endif
  xTaskCreatePinnedToCore(display_task_0, "display_task_0", 10000, NULL, /*priority*/ 3, NULL, /*core*/ 0);
  xTaskCreatePinnedToCore(batteryMeasureTask_0, "batteryMeasureTask_0", 10000, NULL, /*priority*/ 1, NULL, 0);

  xDeadmanQueueEvent = xQueueCreate(1, sizeof(DeadmanEvent));
  xDisplayChangeEventQueue = xQueueCreate(1, sizeof(uint8_t));

  button0_init();

  add_comms_state_transitions();

  while (!display_task_initialised)
  {
    vTaskDelay(10);
  }
}

void loop()
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

  comms_state_fsm.run_machine();

  nrf24.update();

  button0.loop();

  vTaskDelay(1);
}
//------------------------------------------------------------------

#endif