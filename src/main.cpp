#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <Wire.h>
#include <TaskScheduler.h>
#include <VescData.h>
#include <elapsedMillis.h>

// prototypes
void packet_available_cb(uint16_t from_id);

#include "nrf.h"
#include "trigger.h"

#define BUTTON0_PIN 0

// #define USE_TEST_VALUES
#ifdef USE_TEST_VALUES
#define SEND_TO_BOARD_INTERVAL 1000
#define BOARD_COMMS_TIMEOUT   SEND_TO_BOARD_INTERVAL + 100
#define READ_TRIGGER_INTERVAL 200
#define REQUEST_FROM_BOARD_INTERVAL 500
#define REQUEST_FROM_BOARD_INITIAL_INTERVAL 500
// #define BOARD_FSM_TRIGGER_DEBUG_ENABLED   1
// #define DEBUG_BOARD_PRINT_STATE_NAME     1
// #define TRIGGER_DEBUG_ENABLED 1
// #define PACKET_RECV_DEBUG_ENABLED   1
// #define DEBUG_PRINT_STATE_NAME_ENABLED   1
#else
#define SEND_TO_BOARD_INTERVAL 200
#define BOARD_COMMS_TIMEOUT   SEND_TO_BOARD_INTERVAL + 100
#define READ_TRIGGER_INTERVAL 50
#define REQUEST_FROM_BOARD_INITIAL_INTERVAL 500
#define REQUEST_FROM_BOARD_INTERVAL 3000
#endif

#define BATTERY_VOLTAGE_FULL 4.2 * 11         // 46.2
#define BATTERY_VOLTAGE_CUTOFF_START 3.4 * 11 // 37.4
#define BATTERY_VOLTAGE_CUTOFF_END 3.1 * 11   // 34.1

//------------------------------------------------------------------

elapsedMillis since_waiting_for_response = 0;
bool received_response = true;

VescData old_vescdata;

ControllerData old_packet;

uint16_t missedPacketCounter = 0;
bool serverOnline = false;
uint8_t board_first_packet_count = 0, old_board_first_packet_count = 0;

unsigned long lastPacketId = 0;
unsigned long sendCounter = 0;
bool syncdWithServer = false;
uint8_t rxCorrectCount = 0;

// prototypes

#include "utils.h"
#include <screens.h>
#include "SSD1306.h"
#include <state_machine.h>
#include <board_state.h>

// queues
xQueueHandle xTriggerReadQueue;
xQueueHandle xSendToBoardQueue;

//------------------------------------------------------------------

Button2 button0(BUTTON0_PIN);

void button0_pressed(Button2 &btn)
{
  TRIGGER(EV_BUTTON_CLICK, NULL);
}

void button0_released(Button2 &btn)
{
}

//------------------------------------------------------------------

#define OTHER_CORE 0
#define NORMAL_CORE 1
SemaphoreHandle_t xControllerPacketSemaphore;
SemaphoreHandle_t xCore1Semaphore;

void triggerTask_0(void *pvParameters)
{
  xControllerPacketSemaphore = xSemaphoreCreateMutex();

  Serial.printf("triggerTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    uint8_t e;
    bool read = xQueueReceive(xTriggerReadQueue, &e, (TickType_t)0) == pdTRUE;
    if (read == true)
    {
      if (xControllerPacketSemaphore != NULL && xSemaphoreTake(xControllerPacketSemaphore, (TickType_t)10) == pdTRUE)
      {
        uint8_t old_throttle = nrf24.controllerPacket.throttle;

        nrf24.controllerPacket.throttle = get_mapped_calibrated_throttle();
        if (old_throttle != nrf24.controllerPacket.throttle)
        {
#ifdef TRIGGER_DEBUG_ENABLED
          DEBUGVAL(nrf24.controllerPacket.throttle);
#endif
        }
        xSemaphoreGive(xControllerPacketSemaphore);
      }
      else
      {
        DEBUG("Can't take semaphore!");
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//--------------------------------------------------------------------------------

Scheduler runner;

Task t_ReadTrigger_1(
    READ_TRIGGER_INTERVAL,
    TASK_FOREVER,
    [] {
      uint8_t e = 1;
      xQueueSendToFront(xTriggerReadQueue, &e, pdMS_TO_TICKS(5));
    });

Task t_SendToBoard_1(
    SEND_TO_BOARD_INTERVAL,
    TASK_FOREVER,
    [] {
      uint8_t e = 1;
      xQueueSendToFront(xSendToBoardQueue, &e, pdMS_TO_TICKS(5));
    });

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

void packet_available_cb(uint16_t from_id)
{
  board_id = from_id;
#ifdef PACKET_RECV_DEBUG_ENABLED
  DEBUGVAL("packet_available_cb", from_id, nrf24.boardPacket.id);
#endif
  if (nrf24.boardPacket.id != nrf24.controllerPacket.id)
  {
    // DEBUGVAL("ids don't match", nrf24.controllerPacket.id, nrf24.boardPacket.id);
  }

  if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)10) == pdTRUE)
  {
    TRIGGER(EV_RECV_PACKET, NULL);
    BD_TRIGGER(EV_BD_RESPONDED, "EV_BD_RESPONDED");

#ifdef PACKET_RECV_DEBUG_ENABLED
    DEBUGVAL(reason_toString(nrf24.boardPacket.reason));
#endif
    switch (nrf24.boardPacket.reason)
    {
      case REQUESTED:
#ifdef PACKET_RECV_DEBUG_ENABLED
        DEBUGVAL("REQUESTED", nrf24.boardPacket.id);
#endif
        if (nrf24.boardPacket.id != nrf24.controllerPacket.id)
        {
          // DEBUG("ids don't match!");
        }
        break;
      case BOARD_MOVING:
        TRIGGER(EV_STARTED_MOVING, NULL);
        break;
      case BOARD_STOPPED:
        TRIGGER(EV_STOPPED_MOVING, NULL);
        break;
      case FIRST_PACKET:
        TRIGGER(EV_BOARD_FIRST_CONNECT, "EV_BOARD_FIRST_CONNECT");
        break;
    }

    memcpy(&old_vescdata, &nrf24.boardPacket, sizeof(VescData));

    xSemaphoreGive(xCore1Semaphore);
  }
}

void send_packet_to_board()
{
  nrf24.controllerPacket.id++;
  nrf24.sendPacket(board_id);
  nrf24.controllerPacket.command &= ~COMMAND_REQUEST_UPDATE;

  // esp_err_t result;
  // const uint8_t *addr = peer.peer_addr;
  // controller_packet.command = 0;

  // if (since_last_requested_update > 5000)
  // {
  //   since_last_requested_update = 0;
  //   TRIGGER(EV_REQUESTED_UPDATE, NULL);
  //   controller_packet.command = COMMAND_REQUEST_UPDATE;
  // }

  // if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)100) == pdTRUE)
  // {
  //   controller_packet.throttle = easing.GetValue();
  //   controller_packet.id = sendCounter;

  //   uint8_t bs[sizeof(controller_packet)];
  //   memcpy(bs, &controller_packet, sizeof(controller_packet));

  //   result = esp_now_send(addr, bs, sizeof(bs));
  //   xSemaphoreGive(xCore1Semaphore);

  //   print_throttle(target_throttle);
  // }
  // else
  // {
  //   DEBUG("Unable to take semaphore");
  //   return;
  // }

  // printStatus(result, /*printSuccess*/ false);

  // if (result == ESP_OK)
  // {
  //   sendCounter++;
  // }
  // else
  // {
  //   //DEBUGVAL("Error", sendCounter++);
  // }
}
//--------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  powerpins_init();
  button_init();

  bool nrf_ok = nrf_setup();

#ifdef USE_TEST_VALUES
  Serial.printf("\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("/*               WARNING: Using test values!            */\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("\n");
#endif

  addFsmTransitions();
  add_board_fsm_transitions();

  Wire.begin();
  delay(10);

  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
  delay(100);

  xTaskCreatePinnedToCore(triggerTask_0, "triggerTask_0", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xTriggerReadQueue = xQueueCreate(1, sizeof(uint8_t));
  xSendToBoardQueue = xQueueCreate(1, sizeof(uint8_t));

  xCore1Semaphore = xSemaphoreCreateMutex();

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

  runner.startNow();
  runner.addTask(t_ReadTrigger_1);
  runner.addTask(t_SendToBoard_1);
  t_ReadTrigger_1.enable();
  t_SendToBoard_1.enable();
}
//------------------------------------------------------------------

void loop()
{
  button0.loop();

  runner.execute();

  fsm.run_machine();

  board_fsm.run_machine();

  nrf24.update();

  uint8_t e;
  BaseType_t xStatus = xQueueReceive(xSendToBoardQueue, &e, pdMS_TO_TICKS(0));
  if (xStatus == pdPASS)
  {
    send_packet_to_board();
  } 
}
