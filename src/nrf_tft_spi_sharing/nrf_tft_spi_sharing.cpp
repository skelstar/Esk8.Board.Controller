#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>

// #define TFT_MOSI 19
// #define TFT_SCLK 18
// #define TFT_CS 5
// #define TFT_DC 16
// #define TFT_RST 23
// #define TFT_BL 4 // Display backlight control pin

#define DEFAULT_DISP_TYPE DISP_TYPE_ST7789V
#define DEFAULT_TFT_DISPLAY_WIDTH 240
#define DEFAULT_TFT_DISPLAY_HEIGHT 320
#define DISP_COLOR_BITS_24 0x66
#define DEFAULT_GAMMA_CURVE 0
#define DEFAULT_SPI_CLOCK 20000000
#define TFT_INVERT_ROTATION 0
#define TFT_INVERT_ROTATION1 1
#define TFT_RGB_BGR 0x00

#define USE_TOUCH TOUCH_TYPE_NONE

// #define PIN_NUM_MISO 0  // SPI MISO
// #define PIN_NUM_MOSI 19 // SPI MOSI
// #define PIN_NUM_CLK 18  // SPI CLOCK pin
// #define PIN_NUM_CS 5    // Display CS pin
// #define PIN_NUM_DC 16   // Display command/data pin
// #define PIN_NUM_TCS 0   // Touch screen CS pin

// #define PIN_NUM_RST 23 // GPIO used for RESET control
// #define PIN_NUM_BCKL 4 // GPIO used for backlight control
// #define PIN_BCKL_ON 1  // GPIO value for backlight ON
// #define PIN_BCKL_OFF 0 // GPIO value for backlight OFF

#include <TFT_eSPI.h>

#define NRF_MISO 23 // Orange
#define NRF_MOSI 19 // Blue
#define NRF_CLK 18  // Yellow
#define NRF_CE 26
#define NRF_CS 33

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

VescData board_packet;

NRF24L01Lib nrf24;

RF24 radio = RF24(/*sclk*/ 18, /*miso*/ 23, /*mosi*/ 19, NRF_CE, NRF_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

elapsedMillis since_sent_to_board;

//------------------------------------------------------------------

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library

//------------------------------------------------------------------

void packet_available_cb(uint16_t from_id, uint8_t type)
{
  ControllerData board_packet;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&board_packet, &buff, sizeof(ControllerData));

  DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
}
//------------------------------------------------------------------

void init_tft()
{
  tft.init();
  tft.setRotation(1);       // 0 is portrait
  tft.fillScreen(TFT_BLUE); // Clear screen
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);

  DEBUG("setup_tft()");
}

void init_nrf()
{
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.printDetails();

  // nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);
}

void setup()
{
  Serial.begin(115200);

  init_nrf();
  delay(1000);
  init_tft();

  DEBUG("Ready to rx from board...");
}

int i = 0;

void loop()
{
  if (since_sent_to_board > 5000)
  {
    since_sent_to_board = 0;
    DEBUG("sending..");

    tft.drawNumber(i, 20, 20);
    i++;

    uint8_t bs[sizeof(VescData)];
    memcpy(bs, &board_packet, sizeof(VescData));

    // uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ 0, bs, sizeof(VescData), NUM_RETRIES);
    // if (retries > 0)
    // {
    //   DEBUGVAL(retries);
    // }
    // board_packet.id++;
  }

  // nrf24.update();
}