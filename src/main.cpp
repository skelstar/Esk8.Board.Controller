#if (defined(__AVR__))
    #include <avr\ pgmspace.h> 
#else
    #include <pgmspace.h> 
#endif
#include <Arduino.h>
#include <SPI.h>
#include <NRF24L01Library.h>

NRF24L01Lib nrf;

#define SPI_CE        5 	//33    // white/purple
#define SPI_CS        13	//26  // green

RF24 radio(SPI_CE, SPI_CS);    // ce pin, cs pinRF24Network network();
RF24Network network(radio); 


void packet_cb( uint16_t from ) {
	// lastAckFromBoard = millis();
	// debug.print(COMMUNICATION, "Rx from Board: batt voltage %.1f, vescOnline: %d \n", 
	// 	esk8.boardPacket.batteryVoltage, 
	// 	esk8.boardPacket.vescOnline);
	// updateDisplay();
}

void setup() {

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf.begin(&radio, &network, nrf.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  // WiFi.mode( WIFI_OFF );	// WIFI_MODE_NULL
	// btStop();   // turn bluetooth module off
}

void loop() {
  nrf.update();
}