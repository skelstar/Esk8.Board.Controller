#ifdef SERIAL__DEBUG
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <VescData.h>
#include <SPI.h>
#include <NRFLite.h>
#include <TFT_eSPI.h>

// #define TFT_MOSI  19   // for hardware SPI data pin (all of available pins)
// #define TFT_SCLK  18   // for hardware SPI sclk pin (all of available pins)
#define TFT_CS 5 // only for displays with CS pin
#define TFT_DC 16
#define TFT_RST 23

#define NRF_MOSI 23 // blue?
#define NRF_MISO 19 // orange?
#define NRF_CLK 18  // yellow
#define NRF_CS 33   // green
#define NRF_CE 26   // same as tft 15 // white

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

const uint16_t Display_Color_Black = 0x0000;
const uint16_t Display_Color_Blue = 0x001F;
const uint16_t Display_Color_Red = 0xF800;
const uint16_t Display_Color_Green = 0x07E0;
const uint16_t Display_Color_Cyan = 0x07FF;
const uint16_t Display_Color_Magenta = 0xF81F;
const uint16_t Display_Color_Yellow = 0xFFE0;
const uint16_t Display_Color_White = 0xFFFF;
//------------------------------------------------------------------

// NRF24L01Lib nrf24;

// RF24 radio(NRF_CE, NRF_CS);
// RF24Network network(radio);

NRFLite _radio(Serial);

const static uint8_t RADIO_ID = 0;             // Our radio's id.
const static uint8_t DESTINATION_RADIO_ID = 1; // Id of the radio we will transmit to.

TFT_eSPI tft = TFT_eSPI(135, 240);

elapsedMillis since_sent_to_board;

//------------------------------------------------------------

void init_tft()
{
  DEBUG("-----------------------\nsetup_tft()\n-----------------------");

  tft.init();
  tft.setRotation(1);       // 0 is portrait
  tft.fillScreen(TFT_BLUE); // Clear screen
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);
}

//------------------------------------------------------------

void init_nrf()
{
  DEBUG("-----------------------\nsetup_nrf()\n-----------------------");
  _radio.init(RADIO_ID, NRF_MISO, NRF_MOSI, NRF_CLK, NRF_CE, NRF_CS, NRFLite::BITRATE250KBPS, 100);
  _radio.printDetails();
}

//------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(100);

  init_tft();
  delay(1000);
  init_nrf();
}
//------------------------------------------------------------------

VescData vesc_data;

void loop()
{
  while (_radio.hasData())
  {
    _radio.readData(&vesc_data);

    DEBUGVAL("packet!", vesc_data.id);
  }

  vTaskDelay(100);
}
//------------------------------------------------------------------
