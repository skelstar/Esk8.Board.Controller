#ifdef SERIAL__DEBUG
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <Smoothed.h>

#include <SSD1306.h>

#define SPI_CE 33
#define SPI_CS 26

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
    unsigned long num_packets_with_retries;
} stats;

#define NUM_RETRIES 5
#define SEND_TO_BOARD_INTERVAL 200

elapsedMillis since_sent_to_board;
elapsedMillis since_read_trigger;

bool throttle_enabled = true;

//------------------------------------------------------------

Smoothed <float> retry_log;

#define READ_TRIGGER_PERIOD 200
#define SMOOTH_OVER_MILLIS 2000

//------------------------------------------------------------

#include <comms_2.h>
#include <TriggerLib.h>
#include <peripherals.h>
#include <utils.h>
#include <features/deadman.h>

#include <core0.h>

TriggerLib trigger(/*pin*/ 13, /*deadzone*/ 10);

void read_trigger()
{
  uint8_t old_throttle = controller_packet.throttle;

#ifdef FEATURE_PUSH_TO_ENABLE
  controller_packet.throttle = trigger.get_push_to_start_throttle(trigger.get_throttle(), board_packet.moving);
#else
  controller_packet.throttle = trigger.get_throttle();
#endif

#ifdef PRINT_THROTTLE
  if (old_throttle != controller_packet.throttle)
  {
    DEBUGVAL(controller_packet.throttle);
    old_throttle = controller_packet.throttle;
  }
#endif
}

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  #define LOG_LENGTH_MILLIS 5000
  retry_log.begin(SMOOTHED_AVERAGE, LOG_LENGTH_MILLIS / SEND_TO_BOARD_INTERVAL);

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
  controller_config.throttle_smoothing_period = 2000; // ignored for now
  send_config_packet_to_board();

  trigger.initialise();

  // button_init();

  // core 0
  xTaskCreatePinnedToCore(deadmanTask_0, "deadmanTask_0", 4092, NULL, /*priority*/ 4, NULL, 0);
  xTaskCreatePinnedToCore(display_task_0, "display_task_0", 10000, NULL, /*priority*/ 3, NULL, /*core*/ 0);
  xTaskCreatePinnedToCore(batteryMeasureTask_0, "batteryMeasureTask_0", 10000, NULL, /*priority*/ 1, NULL, 0);

  DEBUG("Ready to rx from board...and stuff");
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

  nrf24.update();

  button0.loop();

  vTaskDelay(10);
}
//------------------------------------------------------------------
