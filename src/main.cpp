#if (defined(__AVR__))
    #include <avr\ pgmspace.h> 
#else
    #include <pgmspace.h> 
#endif
#include <Arduino.h>
#include <SPI.h>
#include <NRF24L01Library.h>

NRF24L01Lib nrf;

#define SPI_CE        33    	// white/purple
#define SPI_CS        26  	// green

RF24 radio(SPI_CE, SPI_CS);    // ce pin, cs pinRF24Network network();
RF24Network network(radio); 

void packet_cb( uint16_t from ) {
  Serial.printf("packet_cb(%d)\n", from);
}

void sendToServer() {
  nrf.controllerPacket.throttle += 1;
  if (nrf.controllerPacket.throttle > 127) {
    nrf.controllerPacket.throttle = -127;
  }
  bool success = nrf.sendPacket(nrf.RF24_SERVER);
  if (success) {
    Serial.printf("Sent OK\n");
  }
  else {
    Serial.printf("Failed to send\n");
  }
}
//------------------------------------------------------------------
void setup() {

  Serial.begin(115200);

  SPI.begin();
  radio.begin();
  nrf.begin(&radio, &network, nrf.RF24_CLIENT, packet_cb);
  radio.setAutoAck(true);

  // WiFi.mode( WIFI_OFF );	// WIFI_MODE_NULL
	// btStop();   // turn bluetooth module off
}
//------------------------------------------------------------------

long now = 0;

void loop() {
  nrf.update();

  if (millis() - now > 2000) {
    now = millis();
    sendToServer();
  }
}
//------------------------------------------------------------------
