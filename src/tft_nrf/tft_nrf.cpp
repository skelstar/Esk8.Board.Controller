#ifdef SERIAL__DEBUG
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>
#include <VescData.h>

#include <SPI.h>
#include <TFT_eSPI.h>

// #define TFT_MOSI  19   // for hardware SPI data pin (all of available pins)
// #define TFT_SCLK  18   // for hardware SPI sclk pin (all of available pins)
#define TFT_CS    5 // only for displays with CS pin
#define TFT_DC    16
#define TFT_RST   23 

// HSPI
#define NRF_MOSI 13 // blue?
#define NRF_MISO 12 // orange?
#define NRF_CLK 15  // yellow
#define NRF_CS 33 // green
#define NRF_CE 26 // white
// #define NRF_MOSI 23 // blue?
// #define NRF_MISO 19 // orange?
// #define NRF_CLK 18  // yellow
// #define NRF_CS 12 // green
// #define NRF_CE 16 // same as tft 15 // white

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

const uint16_t  Display_Color_Black        = 0x0000;
const uint16_t  Display_Color_Blue         = 0x001F;
const uint16_t  Display_Color_Red          = 0xF800;
const uint16_t  Display_Color_Green        = 0x07E0;
const uint16_t  Display_Color_Cyan         = 0x07FF;
const uint16_t  Display_Color_Magenta      = 0xF81F;
const uint16_t  Display_Color_Yellow       = 0xFFE0;
const uint16_t  Display_Color_White        = 0xFFFF;
//------------------------------------------------------------------
#include <SPI.h>
#include <NRFLite.h>

VescData vesc_data;

NRFLite _radio(Serial);

void packet_available_cb(uint16_t from_id, uint8_t type)
{
}

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

elapsedMillis since_sent_to_board;

//------------------------------------------------------------

#include <tft.h>

void init_tft()
{
  DEBUG("setup_tft()");
  setup_tft();
}

//------------------------------------------------------------

void init_nrf()
{
  DEBUG("setup_nrf()");
  _radio.init(0, NRF_CE, NRF_CS, NRFLite::BITRATE250KBPS);
  _radio.printDetails();
}

//------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(100);

  
  init_tft();
  delay(100);
  init_nrf();

}
//------------------------------------------------------------------
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
