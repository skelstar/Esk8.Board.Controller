#include <pgmspace.h>
#include <Arduino.h>
#include <SPI.h>
#include <NRF24L01Library.h>
#include <Wire.h>
// #include <i2cEncoderLib.h>

#define 	OLED_SCL		15
#define 	OLED_SDA		4
#define   OLED_RST    16
#include "SSD1306.h"
//------------------------------------------------------------------
NRF24L01Lib nrf24;

#define SPI_MOSI  23  // blue
#define SPI_MISO  19  // orange
#define SPI_CLK   18  // yellow
#define SPI_CE 33     // white/purple
#define SPI_CS 26     // green

RF24 radio(SPI_CE, SPI_CS); // ce pin, cs pinRF24Network network();
RF24Network network(radio);

bool idFromBoardExpected(long id);

#define NO_PACKET_RECEIVED_FROM_BOARD -1
long lastIdFromBoard = NO_PACKET_RECEIVED_FROM_BOARD;
uint8_t missedPacketsCounter = 0;

#include "utils.h"
//------------------------------------------------------------------
void updateDisplayWithMissedPacketCount() {
  #ifdef USING_SSD1306
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  char buffx[16];
  sprintf(buffx, "Missed: %d", missedPacketsCounter);
  u8g2.setFont(FONT_SIZE_MED); // full
  int width = u8g2.getStrWidth(buffx);
  u8g2.drawStr(LCD_WIDTH/2-width/2, LCD_HEIGHT/2-FONT_SIZE_MED_LINE_HEIGHT/2, buffx);
  u8g2.sendBuffer();
  #endif
}

xQueueHandle xThrottleChangeQueue;

enum EventEnum
{
  EVENT_THROTTLE_CHANGED = 1,
  EVENT_2,
  EVENT_3
} event;

//------------------------------------------------------------------
void sendToServer()
{
  if (!idFromBoardExpected(nrf24.boardPacket.id)) {
    Serial.printf("Id '%u' from board not expected\n", nrf24.controllerPacket.id);
    missedPacketsCounter++;
    updateDisplayWithMissedPacketCount();
  }

  nrf24.controllerPacket.id = nrf24.boardPacket.id;
  lastIdFromBoard = nrf24.boardPacket.id;
  bool success = nrf24.sendPacket(nrf24.RF24_SERVER);
  if (success) {
    Serial.printf("Replied to %u OK (with %u)\n", 
      nrf24.boardPacket.id, 
      nrf24.controllerPacket.id);
  }
  else
  {
    Serial.printf("Failed to send\n");
  }
}

void packet_cb( uint16_t from ) {
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

//--------------------------------------------------------------------------------
// #define ENCODER_PWR_PIN 5

void encoderChangedEventCallback(int value) {
  Serial.printf("Encoder: %d\n", value);
}

void encoderPressedEventCallback() {
  Serial.printf("Encoder button pressed!\n");
}

bool getCanAccelerateCallback() {
  return true;
}

// #include "encoder.h"

//--------------------------------------------------------------------------------
void setup()
{

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf24.begin(&radio, &network, nrf24.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  Wire.begin();
  
  // i2cScanner();

  #ifdef USING_ENCODER
  if (setupEncoder(20, -10) == false) 
  {
    Serial.printf("Count not find encoder! \n");
  }
  #endif

  xTaskCreatePinnedToCore(coreTask, "coreTask", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(encoderTask, "encoderTask", 10000, NULL, /*priority*/ 1, NULL, OTHER_CORE);

  xThrottleChangeQueue = xQueueCreate(1, sizeof(EventEnum));

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

  #ifdef USING_SSD1306
  //https://www.aliexpress.com/item/32871318121.html

  setupLCD();
  #endif

  // WiFi.mode( WIFI_OFF );	// WIFI_MODE_NULL
	// btStop();   // turn bluetooth module off

  updateDisplayWithMissedPacketCount();
}
//------------------------------------------------------------------

long now = 0;
BaseType_t xStatus;
const TickType_t xTicksToWait = pdMS_TO_TICKS(50);

void loop()
{

  nrf24.update();

  #ifdef USING_ENCODER
  encoderUpdate();
  #endif

  EventEnum e;
  xStatus = xQueueReceive(xThrottleChangeQueue, &e, xTicksToWait);
  if (xStatus == pdPASS)
  {
    switch (e)
    {
    case EVENT_THROTTLE_CHANGED:
      // Serial.printf("Throttle changed!\n");
      break;
    case EVENT_2:
      // Serial.printf("Event %d\n", e);
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
