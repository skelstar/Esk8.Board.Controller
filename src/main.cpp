#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <Wire.h>
#include <TaskScheduler.h>
#include <VescData.h>
#include <elapsedMillis.h>

#define ENCODER_PWR_PIN 5
#define ENCODER_GND_PIN 17

#define RF24_PWR_PIN 27
#define RF24_GND_PIN 25

#define DEADMAN_INPUT_PIN 0
#define DEADMAN_GND_PIN 12

#define BOARD_COMMS_TIMEOUT       1000
#define MISSED_PACKETS_THRESHOLD  2
#define SEND_TO_BOARD_INTERVAL    200

//------------------------------------------------------------------

VescData vescdata, initialVescData;
ControllerData controller_packet;

uint16_t missedPacketCounter = 0;
bool serverOnline = false;

unsigned long lastPacketId = 0;
unsigned long sendCounter = 0;
bool syncdWithServer = false;
uint8_t rxCorrectCount = 0;

#include "board.h"

Board board;

#include <espNowClient.h>
#include "utils.h"
#include "SSD1306.h"
#include <state_machine.h>

// prototypes

// queues
xQueueHandle xEncoderChangeQueue;
xQueueHandle xDeadmanChangedQueue;
xQueueHandle xStateMachineQueue;
xQueueHandle xSendPacketQueue;

//------------------------------------------------------------------

Button2 deadman(DEADMAN_INPUT_PIN);

void deadmanPressed(Button2 &btn)
{
  bool pressed = true;
  xQueueSendToFront(xDeadmanChangedQueue, &pressed, pdMS_TO_TICKS(10));
}

void deadmanReleased(Button2 &btn)
{
  bool pressed = false;
  xQueueSendToFront(xDeadmanChangedQueue, &pressed, pdMS_TO_TICKS(10));
}

//------------------------------------------------------------------

enum EventEnum
{
  EVENT_THROTTLE_CHANGED = 1,
  EVENT_2,
  EVENT_3
} event;

//--------------------------------------------------------------------------------

#include "encoder.h"

//--------------------------------------------------------------------------------

#define OTHER_CORE 0
#define NORMAL_CORE 1
SemaphoreHandle_t xCore0Semaphore;
SemaphoreHandle_t xCore1Semaphore;

void coreTask_0(void *pvParameters)
{
  Serial.printf("coreTask_0 running on core %d\n", xPortGetCoreID());
  xCore0Semaphore = xSemaphoreCreateMutex();

  while (true)
  {
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void encoderTask_0(void *pvParameters)
{
  Serial.printf("encoderTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    bool accel_enabled = false;
    BaseType_t xStatus;

    /* deadman read */
    xStatus = xQueueReceive(xDeadmanChangedQueue, &accel_enabled, pdMS_TO_TICKS(20));
    if (xStatus == pdPASS)
    {
      DEBUG("xDeadmanChangedQueue");
      updateEncoderMaxCount(accel_enabled);
    }

    /* read encoder */
    if (xCore0Semaphore != NULL && xSemaphoreTake(xCore0Semaphore, (TickType_t)10) == pdTRUE)
    {
      encoderUpdate();
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

void stateMachineTask_1(void *pvParameters)
{
  Serial.printf("stateMachineTask_1 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    BaseType_t xStatus;
    StateMachineEventEnum ev;

    /* deadman read */
    xStatus = xQueueReceive(xStateMachineQueue, &ev, pdMS_TO_TICKS(20));
    if (xStatus == pdPASS)
    {
      DEBUG("xStateMachineQueue");
      fsm.trigger(ev);
    }

    fsm.run_machine();

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
      xQueueSendToFront(xSendPacketQueue, &e, pdMS_TO_TICKS(10));
    });

//--------------------------------------------------------------------------------

void button_init()
{
  deadman.setPressedHandler(deadmanPressed);
  deadman.setReleasedHandler(deadmanReleased);
  deadman.setDoubleClickHandler([](Button2 &b) {
    Serial.printf("deadman.setDoubleClickHandler([](Button2 &b)\n");
  });
  deadman.setTripleClickHandler([](Button2 &b) {
    Serial.printf("deadman.setTripleClickHandler([](Button2 &b)\n");
  });
}
//--------------------------------------------------------------------------------

uint8_t dotsPrinted = 0;

void packetReceived(const uint8_t *data, uint8_t data_len)
{
  if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)10) == pdTRUE)
  {
    fsm.trigger(EV_RECV_PACKET);
    DEBUG("fsm.trigger(EV_RECV_PACKET)");
    memcpy(/*dest*/ &vescdata, /*src*/ data, data_len);
    board.received_packet(vescdata.id);
    xSemaphoreGive(xCore1Semaphore);
  }

  dotsPrinted = printDot(dotsPrinted++);

  // if (board.missed_packets)
  // {
  //   DEBUGVAL("\nMissed packets!", board.lost_packets, board.num_missed_packets);
  //   fsm.trigger(EV_PACKET_MISSED);
  //   rxCorrectCount = 0;
  // }
  // else
  // {
  //   rxCorrectCount++;
  //   if (rxCorrectCount > 10)
  //   {
  //     syncdWithServer = true;
  //     fsm.trigger(EV_SERVER_CONNECTED);
  //   }
  // }
}
//--------------------------------------------------------------------------------

void sendPacket()
{
  esp_err_t result;
  const uint8_t *addr = peer.peer_addr;
  uint8_t bs[sizeof(controller_packet)];

  if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)50) == pdTRUE)
  {
    controller_packet.id = sendCounter;
    memcpy(bs, &controller_packet, sizeof(controller_packet));

    result = esp_now_send(addr, bs, sizeof(bs));
    xSemaphoreGive(xCore1Semaphore);
  }
  else 
  {
    DEBUG("Unable to take sesmaphore");
    return;
  }

  printStatus(result, /*printSuccess*/ false);

  if (result == ESP_OK)
  {
    t_SendToBoard.restart();
    sendCounter++;
  }
  else
  {
    DEBUGVAL("Error", sendCounter++);
  }
}
//--------------------------------------------------------------------------------

void packetSent()
{
}

void board_event(Board::BoardEventEnum ev)
{
  switch (ev)
  {
    case Board::EV_BOARD_TIMEOUT:
      fsm.trigger(EV_BOARD_TIMEOUT);
      break;
    case Board::EV_BOARD_ONLINE:
      fsm.trigger(EV_SERVER_CONNECTED);
      break;
  }
}

//--------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  powerpins_init();
  button_init();

#ifdef USING_SSD1306
  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
#endif

  addFsmTransitions();

  client.setOnConnectedEvent([] {
    Serial.printf("Connected!\n");
  });
  client.setOnDisconnectedEvent([] {
    Serial.println("ESPNow Init Failed, restarting...");
  });
  client.setOnNotifyEvent(packetReceived);
  client.setOnSentEvent(packetSent);
  initESPNow();

  Wire.begin();
  delay(10);

  if (setupEncoder(0, BRAKE_MIN_ENCODER_COUNTS) == false)
  {
    Serial.printf("Count not find encoder! \n");
  }

  xTaskCreatePinnedToCore(coreTask_0, "coreTask_0", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(encoderTask_0, "encoderTask_0", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(stateMachineTask_1, "stateMachineTask_1", 10000, NULL, 1, NULL, NORMAL_CORE);

  xEncoderChangeQueue = xQueueCreate(1, sizeof(EventEnum));
  xDeadmanChangedQueue = xQueueCreate(1, sizeof(bool));
  xStateMachineQueue = xQueueCreate(1, sizeof(StateMachineEventEnum));
  xSendPacketQueue = xQueueCreate(1, sizeof(uint8_t));

  xCore1Semaphore = xSemaphoreCreateMutex();

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

  runner.startNow();
  runner.addTask(t_SendToBoard);
  t_SendToBoard.enable();

  board.setOnEvent(board_event);
}
//------------------------------------------------------------------

void loop()
{
  deadman.loop();

  runner.execute();

  board.loop();

  BaseType_t xStatus;
  EventEnum e;
  xStatus = xQueueReceive(xEncoderChangeQueue, &e, pdMS_TO_TICKS(50));
  if (xStatus == pdPASS)
  {
    switch (e)
    {
    case EVENT_THROTTLE_CHANGED:
    {
      sendPacket();
      t_SendToBoard.restart();
      Serial.printf("Throttle EVENT_THROTTLE_CHANGED! %d\n", controller_packet.throttle);
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
  xStatus = xQueueReceive(xSendPacketQueue, &e2, pdMS_TO_TICKS(50));
  if (xStatus == pdPASS)
  {
    sendPacket();
  }
}
