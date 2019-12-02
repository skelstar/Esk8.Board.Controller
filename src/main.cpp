#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <Wire.h>
#include <TaskScheduler.h>
#include <VescData.h>
#include <state_machine.h>

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
ControllerData controller_packet;
bool serverOnline = false;

#include <espNowClient.h>
#include "utils.h"

// prototypes
void checkConnected();

// queues
xQueueHandle xEncoderChangeQueue;
xQueueHandle xDeadmanChangedQueue;
xQueueHandle xStateMachineQueue;

//------------------------------------------------------------------

Button2 deadman(DEADMAN_INPUT_PIN);

void deadmanPressed(Button2 &btn)
{
  // const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
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

void coreTask_0(void *pvParameters)
{
  Serial.printf("Task running on core %d\n", xPortGetCoreID());
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  xCore0Semaphore = xSemaphoreCreateMutex();

  long other_now = 0;
  while (true)
  {
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

  // DEBUGVAL(rxdata.id);
  Serial.printf(".");

  lastPacketRxTime = millis();

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
  i2cScanner();

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

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

#ifdef USING_SSD1306
  //https://www.aliexpress.com/item/32871318121.html
  setupLCD();
#endif
}
//------------------------------------------------------------------
long timeout = 0;

void loop()
{
  deadman.loop();

  checkConnected();

  if (millis() - timeout > 500)
  {
    timeout = millis();
    const uint8_t *addr = peer.peer_addr;
    controller_packet.id = sendCounter;
    uint8_t bs[sizeof(controller_packet)];
    memcpy(bs, &controller_packet, sizeof(controller_packet));
    esp_err_t result = esp_now_send(addr, bs, sizeof(bs));
    if (result == ESP_OK)
    {
      // DEBUG("Sent timeout packet");
      sendCounter++;
    }
    else
    {
      DEBUG("Some issue with sending timeout packet");
    }
  }

  BaseType_t xStatus;
  EventEnum e;
  xStatus = xQueueReceive(xEncoderChangeQueue, &e, pdMS_TO_TICKS(50));
  if (xStatus == pdPASS)
  {
    switch (e)
    {
    case EVENT_THROTTLE_CHANGED:
    {
      timeout = millis(); // refresh timeout

      const uint8_t *addr = peer.peer_addr;
      controller_packet.id = sendCounter;
      uint8_t bs[sizeof(controller_packet)];
      memcpy(bs, &controller_packet, sizeof(controller_packet));
      esp_err_t result = esp_now_send(addr, bs, sizeof(bs));

      printStatus(result);

      if (result == ESP_OK)
      {
        DEBUGVAL("Sent to Board", controller_packet.id);
        sendCounter++;
      }
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
}
//------------------------------------------------------------------
void checkConnected() 
{
  if (serverOnline == true) 
  {
    if (millis() - lastPacketRxTime > 2000)
    {    
      DEBUG("disconnected");
      serverOnline = false;
      StateMachineEventEnum ev = EV_SERVER_DISCONNECTED;
      xQueueSendToFront(xStateMachineQueue, &ev, pdMS_TO_TICKS(10));
    }  
  }
  else 
  {
    if (millis() - lastPacketRxTime < 2000)
    {
      DEBUG("connected");
      serverOnline = true;
      StateMachineEventEnum ev = EV_SERVER_CONNECTED;
      xQueueSendToFront(xStateMachineQueue, &ev, pdMS_TO_TICKS(10));
    }
  }
}
