#include <pgmspace.h> 
#include <Arduino.h>
#include <SPI.h>
#include <NRF24L01Library.h>
#include <Wire.h>
#include "SSD1306.h"

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

// void scanFori2cDevice() {
//   byte error, address;
//   int nDevices;
 
//   Serial.println("Scanning...");
 
//   nDevices = 0;
//   for(address = 1; address < 127; address++ )
//   {
//     // The i2c_scanner uses the return value of
//     // the Write.endTransmisstion to see if
//     // a device did acknowledge to the address.
//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();
 
//     if (error == 0)
//     {
//       Serial.print("I2C device found at address 0x");
//       if (address<16)
//         Serial.print("0");
//       Serial.print(address,HEX);
//       Serial.println("  !");
 
//       nDevices++;
//     }
//     else if (error==4)
//     {
//       Serial.print("Unknown error at address 0x");
//       if (address<16)
//         Serial.print("0");
//       Serial.println(address,HEX);
//     }    
//   }
//   if (nDevices == 0)
//     Serial.println("No I2C devices found\n");
//   else
//     Serial.println("done\n");
 
//   delay(5000);           // wait 5 seconds for next scan
// }




//------------------------------------------------------------------
void setup() {

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf24.begin(&radio, &network, nrf24.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  #ifdef SSD1306
  Wire.begin();
  setupLCD();
  #endif

  // WiFi.mode( WIFI_OFF );	// WIFI_MODE_NULL
	// btStop();   // turn bluetooth module off
}
//------------------------------------------------------------------

long now = 0;

void loop() {

  nrf24.update();

}
//------------------------------------------------------------------
