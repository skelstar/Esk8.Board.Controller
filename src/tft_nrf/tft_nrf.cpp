#ifdef SERIAL__DEBUG
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <SPI.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>
// #include <TFT_eSPI.h>
#include <Adafruit_GFX.h>
// #include <Arduino_ST7789.h>
#include <Adafruit_ST7789.h>


// #define TFT_MOSI  19   // for hardware SPI data pin (all of available pins)
// #define TFT_SCLK  18   // for hardware SPI sclk pin (all of available pins)
#define TFT_CS    5 // only for displays with CS pin
#define TFT_DC    16
#define TFT_RST   23 

#define NRF_MOSI 23 // blue?
#define NRF_MISO 19 // orange?
#define NRF_CLK 18  // yellow
#define NRF_CS 12 // green
#define NRF_CE 16 // same as tft 15 // white

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

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

void packet_available_cb(uint16_t from_id, uint8_t type)
{
}

// TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
// Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST, TFT_CS);

// #if (SPI_INTERFACES_COUNT == 1)
  SPIClass* spi = &SPI;
// #else
//   SPIClass* spi = &SPI1;
// #endif

Adafruit_ST7789 tft = Adafruit_ST7789(spi, TFT_CS, TFT_DC, TFT_RST);

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
  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);
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
  vTaskDelay(100);
}
//------------------------------------------------------------------
