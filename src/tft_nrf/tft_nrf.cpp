/*

Demonstrates simple RX and TX operation.
Any of the Basic_TX examples can be used as a transmitter.
Please read through 'NRFLite.h' for a description of all the methods available in the library.

Radio    Arduino
CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> No connection
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <SPI.h>
#include <NRFLite.h>

static const uint8_t NRF_MOSI = 13; // blue?
static const uint8_t NRF_MISO = 12; // orange?
static const uint8_t NRF_CLK = 15;  // yellow
static const uint8_t NRF_CS = 33; // green
static const uint8_t NRF_CE = 26; // white

const static uint8_t RADIO_ID = 0;             // Our radio's id.  The transmitter will send to this id.
const static uint8_t DESTINATION_RADIO_ID = 1; // Id of the radio we will transmit to.
// const static uint8_t PIN_RADIO_CSN = 33;
// const static uint8_t PIN_RADIO_CE = 26;

struct RadioPacket // Any packet up to 32 bytes can be sent.
{
  uint8_t FromRadioId;
  uint32_t OnTimeMillis;
  uint32_t FailedTxCount;
};

NRFLite _radio(Serial);
RadioPacket _radioData;

void setup()
{
  Serial.begin(115200);

  // By default, 'init' configures the radio to use a 2MBPS bitrate on channel 100 (channels 0-125 are valid).
  // Both the RX and TX radios must have the same bitrate and channel to communicate with each other.
  // You can run the 'ChannelScanner' example to help select the best channel for your environment.
  // You can assign a different bitrate and channel as shown below.
  //   _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, 100) // THE DEFAULT
  //   _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE1MBPS, 75)
  //   _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, 0)

  delay(500);

  if (!_radio.init(RADIO_ID, NRF_MISO, NRF_MOSI, NRF_CLK, NRF_CE, NRF_CS, NRFLite::BITRATE2MBPS, 100))
  {
    Serial.println("Cannot communicate with radio");
    while (1)
      ; // Wait here forever.
  }
}

#include <elapsedMillis.h>

elapsedMillis since_sent = 0;

void loop()
{
  while (_radio.hasData())
  {
    _radio.readData(&_radioData); // Note how '&' must be placed in front of the variable name.

    String msg = "Radio ";
    msg += _radioData.FromRadioId;
    msg += ", ";
    msg += _radioData.OnTimeMillis;
    msg += " ms, ";
    msg += _radioData.FailedTxCount;
    msg += " Failed TX";

    Serial.println(msg);
  }

  if (since_sent > 1000)
  {
    since_sent = 0;
    _radioData.OnTimeMillis = millis();
    if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData))) // Note how '&' must be placed in front of the variable name.
    {
      Serial.println("...Success");
    }
    else
    {
      Serial.println("...Failed");
      _radioData.FailedTxCount++;
    }
  }
}
