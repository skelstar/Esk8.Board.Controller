#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <rom/rtc.h> // for reset reason
#include <Smoothed.h>

// used in TFT_eSPI library as alternate SPI port (HSPI?)
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define NRF_CE 26
#define NRF_CS 33

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <TFT_eSPI.h>
#include <Wire.h>
#include <Preferences.h>

//------------------------------------------------------------------

#define INDEX_FINGER_PIN 17
#define HALL_EFFECT_SENSOR_PIN 36

//------------------------------------------------------------------

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

VescData board_packet, old_board_packet;
ControllerData controller_packet;
ControllerConfig controller_config;
//------------------------------------------------------------------

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

#define DEADMAN_PIN INDEX_FINGER_PIN

#include <EncoderThrottleLib.h>

EncoderThrottleLib throttle;

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
  RESET_REASON reset_reason_core0;
  RESET_REASON reset_reason_core1;
  uint16_t soft_resets = 0;
} stats;

Preferences storage;

elapsedMillis since_sent_to_board;
elapsedMillis since_read_trigger;

uint16_t remote_battery_percent = 0;

Smoothed<float> retry_log;

#define SMOOTH_OVER_MILLIS 2000

//------------------------------------------------------------

xQueueHandle xDisplayChangeEventQueue;

//------------------------------------------------------------------

#include <utils.h>
#include <screens.h>
#include <menu_system.h>
#include <comms_connected_state.h>
#include <nrf_comms.h>

#include <display_task_0.h>
#include <features/battery_measure.h>
#include <core1.h>

#include <peripherals.h>
#include <Button2.h>

Button2 _deadmanButton(DEADMAN_PIN);

void encoderChanged(i2cEncoderLibV2 *obj)
{
  controller_packet.throttle = throttle.mapCounterToThrottle(_deadmanButton.isPressed());
  DEBUGVAL(obj->readCounterByte(), controller_packet.throttle);
}

void encoderButtonPushed(i2cEncoderLibV2 *obj)
{
  controller_packet.throttle = throttle.mapCounterToThrottle(_deadmanButton.isPressed());
  DEBUGVAL("button pushed!!!", controller_packet.throttle);
}

void deadmanReleased(Button2 &btn)
{
  controller_packet.throttle = throttle.mapCounterToThrottle(/*pressed*/ false);
  DEBUGVAL(controller_packet.throttle);
}

void setup()
{
  Serial.begin(115200);

  storage.begin("stats", /*read-only*/ false);
  stats.soft_resets = storage.getUInt("soft resets", 0);

  stats.reset_reason_core0 = rtc_get_reset_reason(0);
  stats.reset_reason_core1 = rtc_get_reset_reason(1);

  Serial.printf("CPU0 reset reason: %s\n", get_reset_reason_text(stats.reset_reason_core0));
  Serial.printf("CPU1 reset reason: %s\n", get_reset_reason_text(stats.reset_reason_core1));

  if (stats.reset_reason_core0 == RESET_REASON::SW_CPU_RESET)
  {
    stats.soft_resets++;
    storage.putUInt("soft resets", stats.soft_resets);
    DEBUGVAL("RESET!!! =========> ", stats.soft_resets);
  }
  else if (stats.reset_reason_core0 == RESET_REASON::POWERON_RESET)
  {
    stats.soft_resets = 0;
    storage.putUInt("soft resets", stats.soft_resets);
    DEBUG("Storage: cleared resets");
  }
  storage.end();

#define LOG_LENGTH_MILLIS 5000
  retry_log.begin(SMOOTHED_AVERAGE, LOG_LENGTH_MILLIS / SEND_TO_BOARD_INTERVAL);

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  print_build_status();

  // throttle
  Wire.begin();
  throttle.init(encoderChanged, encoderButtonPushed, /*min*/ -ENCODER_NUM_STEPS_BRAKE, /*max*/ ENCODER_NUM_STEPS_ACCEL);
  _deadmanButton.setReleasedHandler(deadmanReleased);

  // core 0
  xTaskCreatePinnedToCore(display_task_0, "display_task_0", 10000, NULL, /*priority*/ 3, NULL, /*core*/ 0);
  xTaskCreatePinnedToCore(batteryMeasureTask_0, "batteryMeasureTask_0", 10000, NULL, /*priority*/ 1, NULL, 0);

  xDisplayChangeEventQueue = xQueueCreate(1, sizeof(uint8_t));

  button0_init();

  add_comms_state_transitions();

  while (!display_task_initialised)
  {
    vTaskDelay(10);
  }
}

elapsedMillis since_sent_config_to_board;

void loop()
{
  if (since_read_trigger > READ_TRIGGER_PERIOD)
  {
    since_read_trigger = 0;

    throttle.loop();

    // read_trigger();
  }

  if (since_sent_to_board > SEND_TO_BOARD_INTERVAL)
  {
    since_sent_to_board = 0;

    if (comms_state_connected == false)
    {
      controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
      send_config_packet_to_board();
    }
    else
    {
      send_control_packet_to_board();
    }
  }

  comms_state_fsm.run_machine();

  nrf24.update();

  button0.loop();

  _deadmanButton.loop();

  vTaskDelay(1);
}
//------------------------------------------------------------------

#endif