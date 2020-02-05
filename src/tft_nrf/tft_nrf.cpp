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

static const uint8_t NRF_MOSI = 13; // blue?
static const uint8_t NRF_MISO = 12; // orange?
static const uint8_t NRF_CLK = 15;  // yellow
static const uint8_t NRF_CS = 33;   // green
static const uint8_t NRF_CE = 26;   // white

const static uint8_t RADIO_ID = 0;             // Our radio's id.  The transmitter will send to this id.
const static uint8_t DESTINATION_RADIO_ID = 1; // Id of the radio we will transmit to.
// const static uint8_t PIN_RADIO_CSN = 33;
// const static uint8_t PIN_RADIO_CE = 26;

//-------------------------------------------------
struct RadioPacket // Any packet up to 32 bytes can be sent.
{
  uint8_t FromRadioId;
  uint32_t OnTimeMillis;
  uint32_t FailedTxCount;
};

NRFLite _radio(Serial);
RadioPacket _radioData;

//-------------------------------------------------
void init_tft()
{
  tft.init();
  tft.setRotation(1);       // 0 is portrait
  tft.fillScreen(TFT_BLUE); // Clear screen
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);
  tft.setRotation(2);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); // Backlight on

  Serial.printf("setup_tft()\n");
}
//-------------------------------------------------
void setup()
{
  Serial.begin(115200);

  delay(500);

  if (!_radio.init(RADIO_ID, NRF_CE, NRF_CS, NRFLite::BITRATE2MBPS, 100))
  // if (!_radio.init(RADIO_ID, NRF_MISO, NRF_MOSI, NRF_CLK, NRF_CE, NRF_CS, NRFLite::BITRATE2MBPS, 100))
  {
    Serial.println("Cannot communicate with radio");
    while (1)
      ; // Wait here forever.
  }

  delay(100);

  init_tft();
}

#include <elapsedMillis.h>

elapsedMillis since_sent = 0, since_waiting = 0;

void loop()
{
  if (since_sent > 1000)
  {
    since_sent = 0;
    _radioData.OnTimeMillis = millis();
    if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData))) // Note how '&' must be placed in front of the variable name.
    {
      Serial.printf("...sent\n");
      since_waiting = 0;

      while (_radio.hasData() == false && since_waiting > 100)
      {
      }

      if (_radio.hasData())
      {
        Serial.printf(" ... rx after %lums round trip\n", (unsigned long)since_waiting);
        _radio.readData(&_radioData); // Note how '&' must be placed in front of the variable name.
      }
      else
      {
        Serial.printf("timed out :(\n");
      }
    }
    else
    {
      Serial.println("...failed to send :(");
      _radioData.FailedTxCount++;
    }
  }
}
