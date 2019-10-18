#include <pgmspace.h> 
#include <Arduino.h>
#include <SPI.h>
#include <NRF24L01Library.h>

NRF24L01Lib nrf24;

#define SPI_CE        33    // white/purple
#define SPI_CS        26  	// green

RF24 radio(SPI_CE, SPI_CS);    // ce pin, cs pinRF24Network network();
RF24Network network(radio); 

bool idFromBoardExpected(long id);

#define NO_PACKET_RECEIVED_FROM_BOARD  -1
long lastIdFromBoard = NO_PACKET_RECEIVED_FROM_BOARD;

xQueueHandle xQueue;
const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
// xQueueSendToBack(xQueue, &e, xTicksToWait);

enum EventEnum
{
  EVENT_1,
  EVENT_2,
  EVENT_3
} event;

//------------------------------------------------------------------
void sendToServer() {

  if (!idFromBoardExpected(nrf24.boardPacket.id)) {
    Serial.printf("Id from board not expected: %l\n", nrf24.controllerPacket.id);
  }
  
  nrf24.controllerPacket.id = nrf24.boardPacket.id;
  lastIdFromBoard = nrf24.boardPacket.id;
  bool success = nrf24.sendPacket(nrf24.RF24_SERVER);
  if (success) {
    Serial.printf("Replied to %u OK (with %u)\n", 
      nrf24.boardPacket.id, 
      nrf24.controllerPacket.id);
  }
  else {
    Serial.printf("Failed to send\n");
  }
}

bool idFromBoardExpected(long id) {
  return lastIdFromBoard == NO_PACKET_RECEIVED_FROM_BOARD 
      || id == lastIdFromBoard + 1;
}

void packet_cb( uint16_t from ) {
  // Serial.printf("packet_cb(%d)\n", from);
  sendToServer();
}
//------------------------------------------------------------------
#define OTHER_CORE 0

void coreTask(void *pvParameters) {

  Serial.printf("Task running on core %d\n", xPortGetCoreID());

  long other_now = 0;
  while(true){
    if (millis() - other_now > 2000) {
      other_now = millis();
      EventEnum e = EVENT_3;
      xQueueSendToBack(xQueue, &e, xTicksToWait);
    }

  }
  vTaskDelete( NULL );
}

void setup() {

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf24.begin(&radio, &network, nrf24.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  xTaskCreatePinnedToCore(coreTask, "coreTask", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);

  xQueue = xQueueCreate(1, sizeof(EventEnum));

  Serial.printf("Loop running on core %d\n", xPortGetCoreID());

  // WiFi.mode( WIFI_OFF );	// WIFI_MODE_NULL
	// btStop();   // turn bluetooth module off
}
//------------------------------------------------------------------

long now = 0;
BaseType_t xStatus;

void loop() {

  nrf24.update();

  EventEnum e;
  xStatus = xQueueReceive( xQueue, &e, xTicksToWait );
  if(xStatus == pdPASS){
    switch (e) {
      case EVENT_1:
        Serial.printf("Event %d\n", e);
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
