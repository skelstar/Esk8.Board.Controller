#include <Arduino.h>
#include <unity.h>


#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#define SPI_CE 33
#define SPI_CS 26

//------------------------------------------------------------------

ControllerData controller_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

//------------------------------------------------------------------

void setUp()
{
}

void tearDown()
{
}

#define NUM_RETRIES 5

void test_send_has_no_retries()
{ 
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  uint8_t retries = nrf24.send_with_retries(00, /*type*/0, bs, sizeof(ControllerData), NUM_RETRIES);
  uint8_t expected = 0;

  TEST_ASSERT_EQUAL(expected, retries);
}

void setup()
{
  UNITY_BEGIN();

  Serial.begin(115200);

  nrf24.begin(&radio, &network, 1, NULL);

  Serial.printf("Ready...");

}

void loop()
{
  RUN_TEST(test_send_has_no_retries);

  UNITY_END();
}
