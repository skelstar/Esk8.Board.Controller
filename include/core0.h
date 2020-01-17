#ifndef Arduino
#include <Arduino.h>
#endif

#include <RF24Network.h>
#include <NRF24L01Library.h>
#include <TriggerLib.h>

#include <comms_fsm.h>

//------------------------------------------------------------
RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);
NRF24L01Lib nrf24;

void board_packet_available_cb(uint16_t from_id, uint8_t type);

#define SEND_TO_BOARD_MS  200
#define NUM_RETRIES       5
#define LOG_LENGTH_MILLIS 5000

void comms_task_0(void *pvParameters)
{
  comms_fsm.run_machine();

  nrf24.begin(&radio, &network, /*address*/ 1, board_packet_available_cb);

  Serial.printf("comms_task_0 running on core %d\n", xPortGetCoreID());

  Smoothed <float> retry_log;
  retry_log.begin(SMOOTHED_AVERAGE, LOG_LENGTH_MILLIS / SEND_TO_BOARD_MS);

  add_comms_fsm_transitions();

  elapsedMillis sent_to_board;

  while (true)
  {
    comms_fsm.run_machine();

    if (since_(sent_to_board, SEND_TO_BOARD_MS, false))
    {
      if (since_(sent_to_board, SEND_TO_BOARD_MS + 10, false))
      {
        DEBUGVAL(sent_to_board);
      }
      sent_to_board = 0;

      controller_packet.id++;
      uint8_t bs[sizeof(ControllerData)];
      memcpy(bs, &controller_packet, sizeof(ControllerData));

      uint8_t retries = nrf24.send_with_retries(00, /*type*/0, bs, sizeof(ControllerData), NUM_RETRIES);
      retry_log.add(retries > 0);

      if (retries > 0)
      {
        DEBUGVAL(retries, retry_log.get());
      }

      controller_packet.command = 0;
    }

    nrf24.update();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void board_packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  DEBUGVAL(board_packet.id);
}

//------------------------------------------------------------

void trigger_read_task_0(void *pvParameters)
{
  elapsedMillis since_read_trigger;
  uint8_t old_throttle = 0;

  #define READ_TRIGGER_PERIOD 100
  #define SMOOTH_OVER_MILLIS  2000

  TriggerLib trigger(10);
  trigger.initialise();
  
  Smoothed <int> sm_throttle;
  sm_throttle.begin(SMOOTHED_EXPONENTIAL, SMOOTH_OVER_MILLIS / READ_TRIGGER_PERIOD);

  Serial.printf("\trigger_read_task_0 running on core %d\n", xPortGetCoreID());

  DEBUGVAL("num smooths", SMOOTH_OVER_MILLIS / READ_TRIGGER_PERIOD);

  while (true)
  {
    if (since_read_trigger > READ_TRIGGER_PERIOD)
    {
      since_read_trigger = 0;

      uint8_t throttle = trigger.get_throttle();
      sm_throttle.add(throttle);
      controller_packet.throttle = sm_throttle.get();

      if (old_throttle != controller_packet.throttle)
      {
        DEBUGVAL(controller_packet.throttle);
        old_throttle = controller_packet.throttle;
      }
    }

    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}