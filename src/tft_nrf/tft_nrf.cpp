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

#define TFT_CS 5 // only for displays with CS pin
#define TFT_DC 16
#define TFT_RST 23

//------------------------------------------------------------------
#include <SPI.h>
#include <myNRFLite.h>

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

myNRFLite _radio(NRF_MISO, NRF_MOSI, NRF_CLK, NRF_CS, NRF_CE, Serial);
RadioPacket _radioData, _rx_data;

//-------------------------------------------------
void init_tft()
{
  tft.init();
  tft.setRotation(1);       // 0 is portrait
  tft.fillScreen(TFT_BLUE); // Clear screen
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); // Backlight on

  Serial.printf("setup_tft()\n");
}

unsigned long counter;
unsigned long timeouts;

void tft_refresh()
{
  char buff[20];
  sprintf(buff, "count: %lu", counter);
  tft.drawString(buff, 20, 20);
  sprintf(buff, "tx/f: %d", _radioData.FailedTxCount);
  tft.drawString(buff, 20, 50);
  sprintf(buff, "t/o: %lu", timeouts);
  tft.drawString(buff, 20, 80);
}
//-------------------------------------------------
void setup()
{
  Serial.begin(115200);

  delay(500);

  if (!_radio.init(RADIO_ID, myNRFLite::BITRATE250KBPS, 0))
  {
    Serial.println("Cannot communicate with radio");
    while (1)
      ; // Wait here forever.
  }
  _radio.printDetails();

  delay(100);

  init_tft();
}

#include <elapsedMillis.h>

elapsedMillis since_sent = 0, since_waiting = 0, since_tft_update;
bool registered_timeout;

void loop()
{
  if (since_tft_update > 2000)
  {
    counter++;
    since_tft_update = 0;
    tft_refresh();
  }

  if (since_sent > 200)
  {
    since_sent = 0;
    _radioData.OnTimeMillis = millis();
    if (!_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData), myNRFLite::NO_ACK)) // Note how '&' must be placed in front of the variable name.
    {
      _radioData.FailedTxCount++;
      tft_refresh();
      DEBUGVAL("...failed to send :(", _radioData.FailedTxCount);
    }
  }

  if (_radio.hasData())
  {
    while (_radio.hasData())
    {
      _radio.readData(&_rx_data);
    }
  }
  else if (since_sent > 50 && !registered_timeout)
  {
    registered_timeout = true;
    timeouts++;
    tft_refresh();
    Serial.printf("timed out :(\n");
  }
}
