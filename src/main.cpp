#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <Wire.h>
#include <TaskScheduler.h>
#include <VescData.h>
#include <espNowClient.h>

#define OLED_SCL 15
#define OLED_SDA 4
#define OLED_RST 16

#define ENCODER_PWR_PIN 5
#define ENCODER_GND_PIN 17

#define RF24_PWR_PIN 27
#define RF24_GND_PIN 25

#define DEADMAN_INPUT_PIN 0
#define DEADMAN_GND_PIN 12

#include "SSD1306.h"
//------------------------------------------------------------------

VescData vescdata, initialVescData;

#include "utils.h"

xQueueHandle xThrottleChangeQueue;
xQueueHandle xDeadmanChangedQueue;

//------------------------------------------------------------------

Button2 deadman(DEADMAN_INPUT_PIN);

void deadmanPressed(Button2 &btn)
{
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  bool pressed = true;
  xQueueSendToFront(xDeadmanChangedQueue, &pressed, xTicksToWait);
}

void deadmanReleased(Button2 &btn)
{
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  bool pressed = false;
  xQueueSendToFront(xDeadmanChangedQueue, &pressed, xTicksToWait);
}

bool canAccelerate(int8_t throttle)
{
  // bool deadmanpressed =  throttle >= 0 && deadman.isPressed();
  bool able = throttle <= 0 ||
         (throttle >= 0 && deadman.isPressed());
  DEBUGVAL("canAccelerate", throttle, able);
  return able;
}

//------------------------------------------------------------------
void updateDisplayWithMissedPacketCount()
{
#ifdef USING_SSD1306
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  char buffx[16];
  // sprintf(buffx, "Missed: %d", missedPacketsCounter);
  u8g2.setFont(FONT_SIZE_MED); // full
  int width = u8g2.getStrWidth(buffx);
  u8g2.drawStr(LCD_WIDTH / 2 - width / 2, LCD_HEIGHT / 2 - FONT_SIZE_MED_LINE_HEIGHT / 2, buffx);
  u8g2.sendBuffer();
#endif
}

enum EventEnum
{
  EVENT_THROTTLE_CHANGED = 1,
  EVENT_2,
  EVENT_3
} event;

//------------------------------------------------------------------
void sendToServer()
{
  // if (!idFromBoardExpected(nrf24.boardPacket.id))
  // {
  //   Serial.printf("Id '%u' from board not expected\n", nrf24.controllerPacket.id);
  //   // missedPacketsCounter++;
  //   updateDisplayWithMissedPacketCount();
  // }

  // nrf24.controllerPacket.id = nrf24.boardPacket.id;
  // lastIdFromBoard = nrf24.boardPacket.id;
  // bool success = nrf24.sendPacket(nrf24.RF24_SERVER);
  // if (success)
  // {
  //   Serial.printf("Replied to %u OK (with %u)\n",
  //                 nrf24.boardPacket.id,
  //                 nrf24.controllerPacket.id);
  // }
  // else
  // {
  //   Serial.printf("Failed to send\n");
  // }
}

// void packet_cb(uint16_t from)
// {
//   sendToServer();
// }
//------------------------------------------------------------------

//--------------------------------------------------------------------------------

#include "encoder.h"

//--------------------------------------------------------------------------------

#define OTHER_CORE 0
SemaphoreHandle_t xCore0Semaphore;

void coreTask_0(void *pvParameters)
{

  Serial.printf("Task running on core %d\n", xPortGetCoreID());
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  xCore0Semaphore = xSemaphoreCreateMutex();

  long other_now = 0;
  while (true)
  {
    // if (millis() - other_now > 200)
    // {
    //   other_now = millis();
    //   EventEnum e = EVENT_2;
    //   xQueueSendToBack(xThrottleChangeQueue, &e, xTicksToWait);
    // }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void encoderTask_0(void *pvParameters)
{
  Serial.printf("Encoder running on core %d\n", xPortGetCoreID());

  while (true)
  {
    bool accel_enabled = false;
    BaseType_t xStatus;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(20);

    xStatus = xQueueReceive(xDeadmanChangedQueue, &accel_enabled, xTicksToWait);
    if (xStatus == pdPASS)
    {
      DEBUG("xDeadmanChangedQueue");
      updateEncoderMaxCount(accel_enabled);
    }

    if (xCore0Semaphore != NULL &&
        xSemaphoreTake(xCore0Semaphore, (TickType_t)10) == pdTRUE)
    {
      encoderUpdate();
      xSemaphoreGive(xCore0Semaphore);
    }
    else
    {
      DEBUG("Can't take semaphore!");
    }
    // bool encoderChanged = millis() - other_now > random(2000);
    // if (encoderChanged)
    // {
    //   other_now = millis();
    //   EventEnum e = EVENT_THROTTLE_CHANGED;
    //   xQueueSendToFront(xThrottleChangeQueue, &e, xTicksToWait);
    // }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void i2cScanner()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  Serial.printf("scanner done, devices found: %d\n", nDevices);
}

void powerpins_init()
{
  // deadman
  pinMode(DEADMAN_GND_PIN, OUTPUT);
  digitalWrite(DEADMAN_GND_PIN, LOW);
  // encoder
  pinMode(ENCODER_PWR_PIN, OUTPUT);
  digitalWrite(ENCODER_PWR_PIN, HIGH);
  pinMode(ENCODER_GND_PIN, OUTPUT);
  digitalWrite(ENCODER_GND_PIN, LOW);
}

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

unsigned long lastPacketRxTime = 0;
unsigned long lastPacketId = 0;
float missedPacketCounter = 0.0;
unsigned long sendCounter = 0;
bool syncdWithServer = false;

void packetReceived(const uint8_t *data, uint8_t data_len)
{
  VescData rxdata;
  memcpy(/*dest*/ &rxdata, /*src*/ data, data_len);

  DEBUGVAL(rxdata.id, sendCounter, lastPacketId, rxdata.batteryVoltage);

  if (lastPacketId != rxdata.id - 1)
  {
    if (syncdWithServer)
    {
      uint8_t lost = (rxdata.id - 1) - lastPacketId;
      missedPacketCounter = missedPacketCounter + lost;
      Serial.printf("Missed %d packets! (%.0f total)\n", lost, missedPacketCounter);
      vescdata.ampHours = missedPacketCounter;
      // fsm.trigger(EV_RECV_PACKET);
    }
  }
  else
  {
    syncdWithServer = true;
  }

  lastPacketId = rxdata.id;
}

void packetSent()
{
  // DEBUGFN("");
}

void setup()
{
  Serial.begin(115200);

  powerpins_init();
  button_init();

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
  i2cScanner();

  if (setupEncoder(20, -10) == false)
  {
    Serial.printf("Count not find encoder! \n");
  }

  xTaskCreatePinnedToCore(coreTask_0, "coreTask_0", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(encoderTask_0, "encoderTask_0", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xThrottleChangeQueue = xQueueCreate(1, sizeof(EventEnum));
  xDeadmanChangedQueue = xQueueCreate(1, sizeof(bool));

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

#ifdef USING_SSD1306
  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
#endif

  // updateDisplayWithMissedPacketCount();
}
//------------------------------------------------------------------

long now = 0;
BaseType_t xStatus;
const TickType_t xTicksToWait = pdMS_TO_TICKS(50);

void loop()
{
  deadman.loop();

  EventEnum e;
  xStatus = xQueueReceive(xThrottleChangeQueue, &e, xTicksToWait);
  if (xStatus == pdPASS)
  {
    switch (e)
    {
    case EVENT_THROTTLE_CHANGED:
      Serial.printf("Throttle EVENT_THROTTLE_CHANGED! %d\n", currentCounter);
      break;
    case EVENT_2:
      // Serial.printf("Event %d\n", e);
      break;
    case EVENT_3:
      // Serial.printf("Event %d\n", e);
      break;
    default:
      Serial.printf("Unhandled event code: %d \n", e);
    }
  }
}
//------------------------------------------------------------------
