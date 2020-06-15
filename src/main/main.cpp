#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <rom/rtc.h> // for reset reason

#include <Adafruit_NeoPixel.h>

// used in TFT_eSPI library as alternate SPI port (HSPI?)
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define NRF_CE 17
#define NRF_CS 2

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <TFT_eSPI.h>
#include <Preferences.h>

//------------------------------------------------------------------

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

VescData board_packet, old_board_packet;
ControllerData controller_packet;
ControllerConfig controller_config;
//------------------------------------------------------------------

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

//------------------------------------------------------------------

#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
#endif
//------------------------------------------------------------------

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library
//------------------------------------------------------------------

class Stats
{
public:
  uint16_t total_failed_sending;
  unsigned long consecutive_resps;
  RESET_REASON reset_reason_core0;
  RESET_REASON reset_reason_core1;
  uint16_t soft_resets = 0;
  uint8_t boardResets = 0;
} stats;

#define STORE_STATS "stats"
#define STORE_STATS_SOFT_RSTS "soft resets"
Preferences statsStore;

elapsedMillis
    since_sent_to_board,
    sinceLastBoardPacketRx,
    sinceSentRequest,
    since_read_trigger;

uint16_t remote_battery_percent = 0;
bool display_task_initialised = false;
bool display_task_showing_option_screen = false;
int oldCounter = 0;

//------------------------------------------------------------

xQueueHandle xDisplayChangeEventQueue;
xQueueHandle xCommsStateEventQueue;

//------------------------------------------------------------------
enum DispStateEvent
{
  DISP_EV_NO_EVENT = 0,
  DISP_EV_CONNECTED,
  DISP_EV_DISCONNECTED,
  DISP_EV_STOPPED,
  DISP_EV_MOVING,
  DISP_EV_UPDATE,
};

// displayState - prototypes
void send_to_display_event_queue(DispStateEvent ev);

//------------------------------------------------------------------

#define ENCODER_BRAKE_COUNTS 20
#define ENCODER_ACCEL_COUNTS 20

class Config
{
public:
  uint8_t headlightMode;
} config;

#define STORE_CONFIG "config"
#define STORE_CONFIG_ACCEL_COUNTS "accel counts"
#define STORE_CONFIG_BRAKE_COUNTS "brake counts"
Preferences configStore;

#include <throttle.h>

ThrottleClass throttle;

//---------------------------------------------------------------

#include <Button2.h>

#include <utils.h>
#include <OptionValue.h>
#include <screens.h>
#include <displayState.h>

#include <display_task_0.h>
#include <comms_connected_state.h>

#include <nrf_comms.h>

#include <features/battery_measure.h>
#include <peripherals.h>

//---------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  statsStore.begin(STORE_STATS, /*read-only*/ false);
  stats.soft_resets = statsStore.getUInt(STORE_STATS_SOFT_RSTS, 0);

  stats.reset_reason_core0 = rtc_get_reset_reason(0);
  stats.reset_reason_core1 = rtc_get_reset_reason(1);

  configStore.begin(STORE_CONFIG, false);

  Serial.printf("CPU0 reset reason: %s\n", get_reset_reason_text(stats.reset_reason_core0));
  Serial.printf("CPU1 reset reason: %s\n", get_reset_reason_text(stats.reset_reason_core1));

  if (stats.reset_reason_core0 == RESET_REASON::SW_CPU_RESET)
  {
    stats.soft_resets++;
    statsStore.putUInt(STORE_STATS_SOFT_RSTS, stats.soft_resets);
    Serial.printf("RESET!!! =========> %d\n", stats.soft_resets);
  }
  else if (stats.reset_reason_core0 == RESET_REASON::POWERON_RESET)
  {
    stats.soft_resets = 0;
    statsStore.putUInt(STORE_STATS_SOFT_RSTS, stats.soft_resets);
    Serial.printf("Storage: cleared resets\n");
  }
  statsStore.end();

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packetAvailable_cb);

  print_build_status();

  throttle.init(/*pin*/ 27);

  primaryButtonInit();

  vTaskDelay(100);

  // core 0
  xTaskCreatePinnedToCore(
      display_task_0,
      "display_task_0",
      10000,
      NULL,
      /*priority*/ 3,
      NULL,
      /*core*/ 0);
  xTaskCreatePinnedToCore(
      commsStateTask_0,
      "commsStateTask_0",
      10000,
      NULL,
      /*priority*/ 2,
      NULL,
      0);
  xTaskCreatePinnedToCore(
      batteryMeasureTask_0,
      "batteryMeasureTask_0",
      10000,
      NULL,
      /*priority*/
      1,
      NULL,
      0);

  xDisplayChangeEventQueue = xQueueCreate(5, sizeof(uint8_t));
  xCommsStateEventQueue = xQueueCreate(5, sizeof(uint8_t));

  while (!display_task_initialised)
  {
    vTaskDelay(1);
  }
}
//---------------------------------------------------------------

elapsedMillis since_sent_config_to_board;
uint8_t old_throttle;

void loop()
{
  if (since_sent_to_board > SEND_TO_BOARD_INTERVAL)
  {
#ifdef PUSH_TO_START
    bool accelEnabled = board_packet.moving;
#else
    bool accelEnabled = true;
#endif
    controller_packet.throttle = throttle.get(/*enabled*/ accelEnabled);
    since_sent_to_board = 0;

    if (comms_state_connected == false)
    {
      controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
      sendConfigToBoard();
    }
    else
    {
#ifdef FEATURE_CRUISE_CONTROL
      controller_packet.cruise_control = primaryButton.isPressed();
#else
      controller_packet.cruise_control = false;
#endif
      sendPacketToBoard();
    }
  }

  if (boardTimedOut())
  {
    sendToCommsEventStateQueue(EV_COMMS_BOARD_TIMEDOUT);
  }

  nrf24.update();

  primaryButton.loop();

  vTaskDelay(1);
}
//------------------------------------------------------------------

#endif