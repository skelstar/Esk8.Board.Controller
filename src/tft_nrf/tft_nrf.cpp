#include <SPI.h>
#include <NRFLite.h>
#include <TFT_eSPI.h>

//-------------------------------------------------

#define TFT_CS 5 // only for displays with CS pin
#define TFT_DC 16
#define TFT_RST 23

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
