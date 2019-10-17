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

void sendToServer() {

  if (!idFromBoardExpected(nrf24.boardPacket.id)) {
    Serial.printf("Id from board not expected: %l\n", nrf24.controllerPacket.id);
  }
  
  nrf24.controllerPacket.id = nrf24.boardPacket.id;
  lastIdFromBoard = nrf24.boardPacket.id;
  bool success = nrf24.sendPacket(nrf24.RF24_SERVER);
  if (success) {
    Serial.printf("Replied to %l OK (with %l)\n", 
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
void setup() {

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf24.begin(&radio, &network, nrf24.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  // WiFi.mode( WIFI_OFF );	// WIFI_MODE_NULL
	// btStop();   // turn bluetooth module off
}
//------------------------------------------------------------------

long now = 0;

void loop() {

  nrf24.update();

}
//------------------------------------------------------------------
