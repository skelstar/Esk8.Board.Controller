#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <Wire.h>
#include <TaskScheduler.h>
// #include <VescData.h>
#include <elapsedMillis.h>

// prototypes
void packet_available_cb(uint16_t from_id);

#include "nrf.h"

#define BUTTON0_PIN 0

#define USE_TEST_VALUES
#ifdef USE_TEST_VALUES
#define CHECK_FOR_BOARD_TIMEOUT 1
#define BOARD_COMMS_TIMEOUT 1000
#define SEND_TO_BOARD_INTERVAL 1000
#define EASING_ACCEL_TIME_INTERVAL 50
#define EASING_ZERO_THROTTLE_PERIOD 200
#else
#define CHECK_FOR_BOARD_TIMEOUT 1
#define BOARD_COMMS_TIMEOUT 1000
#define SEND_TO_BOARD_INTERVAL 200
#define EASING_ACCEL_TIME_INTERVAL 50
#define EASING_ZERO_THROTTLE_PERIOD 200
#endif

#define BATTERY_VOLTAGE_FULL 4.2 * 11         // 46.2
#define BATTERY_VOLTAGE_CUTOFF_START 3.4 * 11 // 37.4
#define BATTERY_VOLTAGE_CUTOFF_END 3.1 * 11   // 34.1

//------------------------------------------------------------------

elapsedMillis since_requested_update = 0;

BoardPacket old_vescdata;

ControllerPacket old_packet;

uint16_t missedPacketCounter = 0;
bool serverOnline = false;
uint8_t board_first_packet_count = 0, old_board_first_packet_count = 0;

unsigned long lastPacketId = 0;
unsigned long sendCounter = 0;
bool syncdWithServer = false;
uint8_t rxCorrectCount = 0;

// prototypes
void TRIGGER(uint8_t x, char *s);

#include "utils.h"
#include <screens.h>
#include <state_machine.h>

void TRIGGER(uint8_t x, char *s)
{
  if (s != NULL)
  {
    Serial.printf("EVENT: %s\n", s);
  }
  fsm.trigger(x);
}

// queues
xQueueHandle xTriggerChangeQueue;
xQueueHandle xDeadmanChangedQueue;
xQueueHandle xSendPacketToBoardQueue;

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

enum EventEnum
{
  EVENT_THROTTLE_CHANGED = 1,
  EVENT_2,
  EVENT_3
} event;

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

#define OTHER_CORE 0
#define NORMAL_CORE 1
SemaphoreHandle_t xCore0Semaphore;
SemaphoreHandle_t xCore1Semaphore;

void triggerTask_0(void *pvParameters)
{
  xCore0Semaphore = xSemaphoreCreateMutex();

  Serial.printf("triggerTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    bool accel_enabled = false;
    BaseType_t xStatus;

    /* deadman read */
    xStatus = xQueueReceive(xDeadmanChangedQueue, &accel_enabled, pdMS_TO_TICKS(20));
    if (xStatus == pdPASS)
    {
      // DEBUG("xDeadmanChangedQueue");
      // updateEncoderMaxCount(accel_enabled);
    }

    /* read encoder */
    if (xCore0Semaphore != NULL && xSemaphoreTake(xCore0Semaphore, (TickType_t)10) == pdTRUE)
    {
      // encoderUpdate();
      xSemaphoreGive(xCore0Semaphore);
    }
    else
    {
      DEBUG("Can't take semaphore!");
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//--------------------------------------------------------------------------------

Scheduler runner;

Task t_SendToBoard(
    SEND_TO_BOARD_INTERVAL,
    TASK_FOREVER,
    [] {
      uint8_t e = 1;
      xQueueSendToFront(xSendPacketToBoardQueue, &e, pdMS_TO_TICKS(5));
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

elapsedMillis since_last_requested_update = 0;

void packet_available_cb(uint16_t from_id)
{
  board_id = from_id;
  DEBUGVAL("packet_available_cb", from_id, nrf24.boardPacket.id);

  if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)10) == pdTRUE)
  {
    if (old_vescdata.isMoving != nrf24.boardPacket.isMoving)
    {
      if (nrf24.boardPacket.isMoving)
      {
        TRIGGER(EV_STARTED_MOVING, NULL);
      }
      else
      {
        TRIGGER(EV_STARTED_MOVING, NULL);
      }
    }

    if (nrf24.boardPacket.id == 0)
    {
      TRIGGER(EV_BOARD_FIRST_CONNECT, NULL);
    }

    memcpy(&old_vescdata, &nrf24.boardPacket, sizeof(BoardPacket));

    xSemaphoreGive(xCore1Semaphore);
  }
}

void send_packet_to_board()
{
  nrf24.sendPacket(board_id);
  nrf24.controllerPacket.id++;

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

// void board_event_cb(Board::BoardEventEnum ev)
// {
//   switch (ev)
//   {
//   case Board::EV_BOARD_TIMEOUT:
//     TRIGGER(EV_BOARD_TIMEOUT, "EV_BOARD_TIMEOUT");
//     break;
//   case Board::EV_BOARD_ONLINE:
//     // TRIGGER(EV_BOARD_CONNECTED, NULL);
//     break;
//   }
// }

//--------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

#ifdef USE_TEST_VALUES
  Serial.printf("\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("/*               WARNING: Using test values!            */\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("\n");
#endif

  powerpins_init();
  button_init();

  bool nrf_ok = nrf_setup();

  addFsmTransitions();

  Wire.begin();
  delay(10);

#ifdef USING_SSD1306
  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
  delay(100);
#endif

  // display_initialise();

  xTaskCreatePinnedToCore(triggerTask_0, "triggerTask_0", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xTriggerChangeQueue = xQueueCreate(1, sizeof(EventEnum));
  xDeadmanChangedQueue = xQueueCreate(1, sizeof(bool));
  xSendPacketToBoardQueue = xQueueCreate(1, sizeof(uint8_t));

  xCore1Semaphore = xSemaphoreCreateMutex();

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

  runner.startNow();
  runner.addTask(t_SendToBoard);
  t_SendToBoard.enable();
}
//------------------------------------------------------------------

void loop()
{
  button0.loop();

  runner.execute();

  fsm.run_machine();

  BaseType_t xStatus;
  EventEnum e;

  nrf24.update();

  xStatus = xQueueReceive(xTriggerChangeQueue, &e, pdMS_TO_TICKS(50));

  if (xStatus == pdPASS)
  {
    switch (e)
    {
    case EVENT_THROTTLE_CHANGED:
    {
      send_packet_to_board();
      t_SendToBoard.restart();
    }
    break;
    case EVENT_2:
      Serial.printf("Event %d\n", e);
      break;
    case EVENT_3:
      Serial.printf("Event %d\n", e);
      break;
    default:
      Serial.printf("Unhandled event code: %d \n", e);
    }
  }

  uint8_t e2;
  xStatus = xQueueReceive(xSendPacketToBoardQueue, &e2, pdMS_TO_TICKS(0));
  if (xStatus == pdPASS)
  {
    send_packet_to_board();
  }
}
