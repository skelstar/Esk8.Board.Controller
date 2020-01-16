#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <Wire.h>
#include <VescData.h>
#include <elapsedMillis.h>

// prototypes
void board_packet_available_cb(uint16_t from_id, uint8_t type);

#define BUTTON0_PIN 0
#define DEADMAN_BUTTON_PIN 5
#define DEADMAN_BUTTON_GND 17

#ifdef USE_TEST_VALUES
#define SEND_TO_BOARD_INTERVAL 1000
#define READ_TRIGGER_INTERVAL 200
#define REQUEST_FROM_BOARD_INTERVAL 3000
#define REQUEST_FROM_BOARD_INITIAL_INTERVAL 500
#define BATTERY_MEASURE_PIN 34
#define BATTERY_MEASURE_PERIOD 1000
#else
#define SEND_TO_BOARD_INTERVAL 200
#define READ_TRIGGER_INTERVAL 50
#define REQUEST_FROM_BOARD_INITIAL_INTERVAL 500
#define REQUEST_FROM_BOARD_INTERVAL 3000
#define BATTERY_MEASURE_PIN 34
#define BATTERY_MEASURE_PERIOD 1000
#endif

#define BATTERY_VOLTAGE_FULL 4.2 * 11         // 46.2
#define BATTERY_VOLTAGE_CUTOFF_START 3.4 * 11 // 37.4
#define BATTERY_VOLTAGE_CUTOFF_END 3.1 * 11   // 34.1

#define REMOTE_BATTERY_FULL 2300
#define REMOTE_BATTERY_EMPTY 1520

//------------------------------------------------------------------

elapsedMillis since_waiting_for_response = 0;
bool received_response = true;

VescData old_vescdata, board_packet;

ControllerData controller_packet;

uint16_t missedPacketCounter = 0;
bool serverOnline = false;
uint8_t board_first_packet_count = 0;
uint16_t remote_battery_volts_raw = 0;
uint8_t remote_battery_percent = 0;

#include "StatsLib.h"

StatsLib stats;

unsigned long lastPacketId = 0;
unsigned long sendCounter = 0;
bool syncdWithServer = false;
uint8_t rxCorrectCount = 0;

bool can_accelerate = true;
uint8_t throttle_unfiltered = 127;

// semaphores
SemaphoreHandle_t xCore1Semaphore;

// queues
xQueueHandle xTriggerReadQueue;
xQueueHandle xSendToBoardQueue;

elapsedMillis since_last_requested_update = 0;
elapsedMillis since_measure_battery = 0;
elapsedMillis since_sent_to_board = 0;

// prototypes

#include "RetryLoggerLib.h"

RetryLoggerLib retry_logger(100);

#include "trigger.h"
#include "utils.h"
#include "nrf.h"
#include "SSD1306.h"
#include <screens.h>
#include <trigger_fsm.h>
#include <state_machine.h>
#include <board_state.h>

//------------------------------------------------------------------

Button2 button0(BUTTON0_PIN);

Button2 deadman_button(DEADMAN_BUTTON_PIN);

void button0_pressed(Button2 &btn)
{
  FSM_EVENT(EV_BUTTON_CLICK, NULL);
}

void button0_released(Button2 &btn)
{
}

void deadman_pressed(Button2 &btn)
{
  TRIGGER_TRIGGER(TRIGGER_DEADMAN_PRESSED, "TRIGGER_DEADMAN_PRESSED");
}

void deadman_released(Button2 &btn)
{
  TRIGGER_TRIGGER(TRIGGER_DEADMAN_RELEASED, "TRIGGER_DEADMAN_RELEASED");
}

#define OTHER_CORE 0
#define NORMAL_CORE 1

//------------------------------------------------------------
void batteryMeasureTask_0(void *pvParameters)
{
  // pinMode(BATTERY_MEASURE_PIN, INPUT);

  while (true)
  {
    if (since_measure_battery > BATTERY_MEASURE_PERIOD)
    {
      since_measure_battery = 0;
      remote_battery_volts_raw = analogRead(BATTERY_MEASURE_PIN);
      remote_battery_percent = get_remote_battery_percent(remote_battery_volts_raw);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
void comms_task_0(void *pvParameters)
{
  bool nrf_ok = nrf_setup();

  bool success = send_controller_packet_to_board();
  if (success == false)
  {
    BD_TRIGGER(EV_BD_TIMEDOUT, "EV_BD_TIMEDOUT");
  }
  else
  {
    FSM_EVENT(EV_SENT_TO_BOARD_OK, "EV_SENT_TO_BOARD_OK");
  }

  controller_packet.id++;

  while (true)
  {
    if (since_sent_to_board > SEND_TO_BOARD_INTERVAL)
    {
      since_sent_to_board = 0;

      if (since_last_requested_update > REQUEST_FROM_BOARD_INTERVAL)
      {
        since_last_requested_update = 0;
        set_request_update_command();
      }

      bool sentOK = send_controller_packet_to_board();

      if (sentOK == false)
      {
        BD_TRIGGER(EV_BD_TIMEDOUT, "EV_BD_TIMEDOUT1");
      }
      else
      {
        FSM_EVENT(EV_SENT_TO_BOARD_OK, "EV_SENT_TO_BOARD_OK");
      }
      controller_packet.id++;
    }

    nrf_update();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//--------------------------------------------------------------------------------

void button_init()
{
  button0.setPressedHandler(button0_pressed);
  button0.setReleasedHandler(button0_released);
  button0.setDoubleClickHandler([](Button2 &b) {
    Serial.printf("button0.setDoubleClickHandler([](Button2 &b)\n");
  });
  button0.setTripleClickHandler([](Button2 &b) {
    Serial.printf("button0.setTripleClickHandler([](Button2 &b)\n");
  });
}

//--------------------------------------------------------------------------------

uint8_t dotsPrinted = 0;

//--------------------------------------------------------------------------------

void board_packet_available_cb(uint16_t from_id, uint8_t type)
{
  board_id = from_id;

  switch (type)
  {
  case 0:
    uint8_t buff[sizeof(VescData)];
    nrf_read(buff, sizeof(VescData));
    memcpy(&board_packet, &buff, sizeof(VescData));

    if (board_packet.reason == ReasonType::REQUESTED)
    {
      BD_TRIGGER(EV_BD_RESPONDED, NULL);
      stats.request_delay_updated = true;
    }
    break;
  case 1:
    DEBUGVAL(type);
    // send_to_packet_controller(ReasonType::REQUESTED);
    break;
  default:
    break;
  }

#ifdef PACKET_RECV_DEBUG_ENABLED
  DEBUGVAL("board_packet_available_cb", from_id, board_packet.id);
#endif
  if (board_packet.id != controller_packet.id)
  {
    // DEBUGVAL("ids don't match", controller_packet.id, board_packet.id);
  }

  if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)10) == pdTRUE)
  {
    FSM_EVENT(EV_RECV_PACKET, NULL);
    BD_TRIGGER(EV_BD_RESPONDED, "EV_BD_RESPONDED");

#ifdef PACKET_RECV_DEBUG_ENABLED
    DEBUGVAL(reason_toString(board_packet.reason));
#endif
    switch (board_packet.reason)
    {
    case REQUESTED:
#ifdef PACKET_RECV_DEBUG_ENABLED
      DEBUGVAL("REQUESTED", board_packet.id);
#endif
      if (board_packet.id != controller_packet.id)
      {
        // DEBUG("ids don't match!");
      }
      break;
    case BOARD_MOVING:
      FSM_EVENT(EV_STARTED_MOVING, NULL);
      break;
    case BOARD_STOPPED:
      FSM_EVENT(EV_STOPPED_MOVING, NULL);
      break;
    case FIRST_PACKET:
      if (controller_packet.id > 10)
      {
        // avoid first 'FIRST_PACKET'
        board_first_packet_count++;
      }
      stats.first_packet_updated = true;
      FSM_EVENT(EV_BOARD_FIRST_CONNECT, "EV_BOARD_FIRST_CONNECT");
      break;
    }

    memcpy(&old_vescdata, &board_packet, sizeof(VescData));

    xSemaphoreGive(xCore1Semaphore);
  }
}
//--------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  powerpins_init();
  button_init();

#ifdef USE_DEADMAN_SWITCH
  pinMode(DEADMAN_BUTTON_GND, OUTPUT);
  digitalWrite(DEADMAN_BUTTON_GND, LOW);
  deadman_button.setPressedHandler(deadman_pressed);
  deadman_button.setReleasedHandler(deadman_released);
#endif

  print_build_status();

  addFsmTransitions();
  add_board_fsm_transitions();

#ifdef USE_DEADMAN_SWITCH
  addTriggerFsmTransitions();
#endif

  Wire.begin();
  delay(10);

  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
  delay(100);

  xTaskCreatePinnedToCore(comms_task_0, "comms_task_0", 10000, NULL, /*priority*/ 4, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(batteryMeasureTask_0, "BATT_0", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xCore1Semaphore = xSemaphoreCreateMutex();

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());
}
//------------------------------------------------------------------

void loop()
{
  button0.loop();

#ifdef USE_DEADMAN_SWITCH
  deadman_button.loop();
#endif

  fsm.run_machine();

  board_fsm.run_machine();

#ifdef USE_DEADMAN_SWITCH
  trigger_fsm.run_machine();
#endif

  read_trigger();
}
