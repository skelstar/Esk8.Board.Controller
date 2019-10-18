#include <pgmspace.h>
#include <Arduino.h>
#include <SPI.h>
#include <NRF24L01Library.h>

NRF24L01Lib nrf24;

#define SPI_CE 33 // white/purple
#define SPI_CS 26 // green

RF24 radio(SPI_CE, SPI_CS); // ce pin, cs pinRF24Network network();
RF24Network network(radio);

bool idFromBoardExpected(long id);

#define NO_PACKET_RECEIVED_FROM_BOARD -1
long lastIdFromBoard = NO_PACKET_RECEIVED_FROM_BOARD;

xQueueHandle xThrottleChangeQueue;

enum EventEnum
{
  EVENT_THROTTLE_CHANGED,
  EVENT_2,
  EVENT_3
} event;

//------------------------------------------------------------------
void sendToServer()
{

  if (!idFromBoardExpected(nrf24.boardPacket.id))
  {
    Serial.printf("Id from board not expected: %l\n", nrf24.controllerPacket.id);
  }

  nrf24.controllerPacket.id = nrf24.boardPacket.id;
  lastIdFromBoard = nrf24.boardPacket.id;
  bool success = nrf24.sendPacket(nrf24.RF24_SERVER);
  if (success)
  {
    Serial.printf("Replied to %u OK (with %u)\n",
                  nrf24.boardPacket.id,
                  nrf24.controllerPacket.id);
  }
  else
  {
    Serial.printf("Failed to send\n");
  }
}

bool idFromBoardExpected(long id)
{
  return lastIdFromBoard == NO_PACKET_RECEIVED_FROM_BOARD || id == lastIdFromBoard + 1;
}

void packet_cb(uint16_t from)
{
  // Serial.printf("packet_cb(%d)\n", from);
  sendToServer();
}
//------------------------------------------------------------------
#define OTHER_CORE 0

void coreTask(void *pvParameters)
{

  Serial.printf("Task running on core %d\n", xPortGetCoreID());
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);

  long other_now = 0;
  while (true)
  {
    if (millis() - other_now > 200)
    {
      other_now = millis();
      EventEnum e = EVENT_2;
      xQueueSendToBack(xThrottleChangeQueue, &e, xTicksToWait);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void encoderTask(void *pvParameters)
{

  Serial.printf("Encoder running on core %d\n", xPortGetCoreID());

  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  long other_now = 0;
  while (true)
  {

    bool encoderChanged = millis() - other_now > random(2000);
    if (encoderChanged)
    {
      other_now = millis();
      EventEnum e = EVENT_THROTTLE_CHANGED;
      xQueueSendToFront(xThrottleChangeQueue, &e, xTicksToWait);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void setup()
{

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf24.begin(&radio, &network, nrf24.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  xTaskCreatePinnedToCore(coreTask, "coreTask", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(encoderTask, "encoderTask", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xThrottleChangeQueue = xQueueCreate(1, sizeof(EventEnum));

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());
}
//------------------------------------------------------------------

long now = 0;
BaseType_t xStatus;
const TickType_t xTicksToWait = pdMS_TO_TICKS(50);

void loop()
{

  nrf24.update();

  EventEnum e;
  xStatus = xQueueReceive(xThrottleChangeQueue, &e, xTicksToWait);
  if (xStatus == pdPASS)
  {
    switch (e)
    {
    case EVENT_THROTTLE_CHANGED:
      Serial.printf("Throttle changed!\n");
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
}
//------------------------------------------------------------------
