#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#define SPI_CE 33
#define SPI_CS 26

#define COMMS_BOARD 00
#define COMMS_CONTROLLER  01

//------------------------------------------------------------------

VescData board_packet;

ControllerData controller_packet;
ControllerConfig controller_config;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

#define NUM_RETRIES 5
#define SEND_TO_BOARD_INTERVAL  500

elapsedMillis since_sent_to_board;

#include <comms_2.h>

//------------------------------------------------------------

elapsedMillis since_read_trigger;
uint8_t old_throttle = 0;

#define READ_TRIGGER_PERIOD 500
#define SMOOTH_OVER_MILLIS  2000

//------------------------------------------------------------

#include <TriggerLib.h>

TriggerLib trigger(/*pin*/13, /*deadzone*/10);

void read_trigger()
{
  controller_packet.throttle = trigger.get_throttle();

  if (old_throttle != controller_packet.throttle)
  {
    DEBUGVAL(controller_packet.throttle);
    old_throttle = controller_packet.throttle;
  }
}

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
  send_config_packet_to_board();

  trigger.initialise();

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

  vTaskDelay(10);
}
//------------------------------------------------------------------
