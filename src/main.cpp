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
#define SEND_TO_BOARD_INTERVAL    200

//------------------------------------------------------------------

elapsedMillis since_requested_update = 0;

VescData vescdata, old_vescdata;
ControllerData controller_packet;

uint16_t missedPacketCounter = 0;
bool serverOnline = false;
uint8_t board_first_packet_count = 0, old_board_first_packet_count = 0;

unsigned long lastPacketId = 0;
unsigned long sendCounter = 0;
bool syncdWithServer = false;
uint8_t rxCorrectCount = 0;

#include "board.h"

// prototypes
void TRIGGER(uint8_t x, char* s);

Board board;

#include <espNowClient.h>
#include "utils.h"
// #include "SSD1306.h"
#include "TTGO_T_Display.h"
#include <screens.h>
#include <state_machine.h>

void TRIGGER(uint8_t x, char* s)
{
  if (s != NULL)
  {
    Serial.printf("EVENT: %s\n", s);
  }
  fsm.trigger(x);
}

// prototypes

// queues
xQueueHandle xEncoderChangeQueue;
xQueueHandle xDeadmanChangedQueue;
xQueueHandle xSendPacketToBoardQueue;

//------------------------------------------------------------------

Button2 deadman(DEADMAN_INPUT_PIN);

void deadmanPressed(Button2 &btn)
{
  bool pressed = true;
  // xQueueSendToFront(xDeadmanChangedQueue, &pressed, pdMS_TO_TICKS(10));
}

void deadmanReleased(Button2 &btn)
{
  bool pressed = false;
  // xQueueSendToFront(xDeadmanChangedQueue, &pressed, pdMS_TO_TICKS(10));
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

void encoderTask_0(void *pvParameters)
{
  xCore0Semaphore = xSemaphoreCreateMutex();

  Serial.printf("encoderTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    bool accel_enabled = false;
    BaseType_t xStatus;

    /* deadman read */
    xStatus = xQueueReceive(xDeadmanChangedQueue, &accel_enabled, pdMS_TO_TICKS(20));
    if (xStatus == pdPASS)
    {
      // DEBUG("xDeadmanChangedQueue");
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
    TRIGGER(EV_RECV_PACKET, NULL);
    memcpy(&old_vescdata, &vescdata, sizeof(vescdata));
    memcpy(&vescdata, data, data_len);
    board.received_packet(vescdata.id);
    xSemaphoreGive(xCore1Semaphore);
    board.num_times_controller_offline = vescdata.ampHours;
  }

  // board's first packet
  if (vescdata.id == 0)
  {
    TRIGGER(EV_BOARD_FIRST_CONNECT, "EV_BOARD_FIRST_CONNECT");
  }
}
//--------------------------------------------------------------------------------

void send_packet_to_board()
{
  esp_err_t result;
  const uint8_t *addr = peer.peer_addr;
  uint8_t bs[sizeof(controller_packet)];

  bool req_update = sendCounter % 50 == 0;
  if (req_update)
  {
    TRIGGER(EV_REQUESTED_UPDATE, NULL);
  }
  controller_packet.command = req_update ? COMMAND_REQUEST_UPDATE : 0;

  if (xCore1Semaphore != NULL && xSemaphoreTake(xCore1Semaphore, (TickType_t)100) == pdTRUE)
  {
    controller_packet.id = sendCounter;
    memcpy(bs, &controller_packet, sizeof(controller_packet));

    result = esp_now_send(addr, bs, sizeof(bs));
    xSemaphoreGive(xCore1Semaphore);
  }
  else 
  {
    DEBUG("Unable to take semaphore");
    return;
  }

  printStatus(result, /*printSuccess*/ false);

  if (result == ESP_OK)
  {
    sendCounter++;
  }
  else
  {
    //DEBUGVAL("Error", sendCounter++);
  }
}
//--------------------------------------------------------------------------------

void packetSent()
{
}

void board_event_cb(Board::BoardEventEnum ev)
{
  switch (ev)
  {
    case Board::EV_BOARD_TIMEOUT:
      TRIGGER(EV_BOARD_TIMEOUT, "EV_BOARD_TIMEOUT");
      break;
    case Board::EV_BOARD_ONLINE:
      // TRIGGER(EV_BOARD_CONNECTED, NULL);
      break;
  }
}

//--------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  powerpins_init();
  button_init();

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

#ifdef USING_SSD1306
  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
  delay(100);
#endif

  display_initialise();

  if (setupEncoder(0, BRAKE_MIN_ENCODER_COUNTS) == false)
  {
    Serial.printf("Count not find encoder! \n");
  }

  xTaskCreatePinnedToCore(encoderTask_0, "encoderTask_0", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xEncoderChangeQueue = xQueueCreate(1, sizeof(EventEnum));
  xDeadmanChangedQueue = xQueueCreate(1, sizeof(bool));
  xSendPacketToBoardQueue = xQueueCreate(1, sizeof(uint8_t));

  xCore1Semaphore = xSemaphoreCreateMutex();

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

  runner.startNow();
  runner.addTask(t_SendToBoard);
  t_SendToBoard.enable();

  board.setOnEvent(board_event_cb);
}
//------------------------------------------------------------------

void loop()
{
  deadman.loop();

  runner.execute();

  board.loop();

  fsm.run_machine();

  BaseType_t xStatus;
  EventEnum e;
  xStatus = xQueueReceive(xEncoderChangeQueue, &e, pdMS_TO_TICKS(50));
  if (xStatus == pdPASS)
  {
    switch (e)
    {
      case EVENT_THROTTLE_CHANGED:
      {
        send_packet_to_board();
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
  xStatus = xQueueReceive(xSendPacketToBoardQueue, &e2, pdMS_TO_TICKS(0));
  if (xStatus == pdPASS)
  {
    send_packet_to_board();
  }
}
