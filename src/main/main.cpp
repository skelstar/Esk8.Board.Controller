#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
// #define PRINTSTREAM_FALLBACK
// #include "Debug.hpp"

#include <Arduino_Helpers.h>
#include <AH/Debug/Debug.hpp>

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <rom/rtc.h> // for reset reason

// used in TFT_eSPI library as alternate SPI port (HSPI?)
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define NRF_CE 17
#define NRF_CS 2

enum ButtonClickType
{
  SINGLE,
  DOUBLE,
  TRIPLE
};

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <TFT_eSPI.h>
#include <Preferences.h>
#include <BatteryLib.h>
#include <EventQueueManager.h>

enum FeatureType
{
  CRUISE_CONTROL,
  PUSH_TO_START
};

#ifndef FEATURE_CRUISE_CONTROL
#define FEATURE_CRUISE_CONTROL false
#endif
#ifndef FEATURE_PUSH_TO_START
#define FEATURE_PUSH_TO_START false
#endif

class FeatureServiceClass
{
public:
  FeatureServiceClass()
  {
    set(CRUISE_CONTROL, FEATURE_CRUISE_CONTROL);
    set(PUSH_TO_START, FEATURE_PUSH_TO_START);
  }

  template <class T>
  void set(FeatureType feature, T value)
  {
    switch (feature)
    {
    case CRUISE_CONTROL:
      _featureCruiseControl = value;
      break;
    case PUSH_TO_START:
      _featurePushToStart = value;
      break;
    }
  }

  template <class T>
  T get(FeatureType feature)
  {
    switch (feature)
    {
    case CRUISE_CONTROL:
      return _featureCruiseControl;
    case PUSH_TO_START:
      return _featurePushToStart;
    }
    return NULL;
  }

private:
  bool _featureCruiseControl;
  bool _featurePushToStart;
} featureService;

//------------------------------------------------------------------

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

ControllerData controller_packet;
ControllerConfig controller_config;

#include <BoardClass.h>

BoardClass board;

//------------------------------------------------------------------

#include <APM.h>

// APM *core0Apm;

//------------------------------------------------------------------

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

//------------------------------------------------------------------

#define BATTERY_MEASURE_PIN 34

BatteryLib remote_batt(BATTERY_MEASURE_PIN);

//------------------------------------------------------------------

#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
#endif
//------------------------------------------------------------------

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

#define TFT_DEFAULT_BG TFT_BLACK

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library
//------------------------------------------------------------------

#define STORE_STATS "stats"
#define STORE_STATS_SOFT_RSTS "soft resets"
#define STORE_STATS_TRIP_TIME "trip time"

Preferences statsStore;

elapsedMillis
    sinceSentToBoard,
    sinceLastBoardPacketRx,
    sinceSentRequest,
    since_read_trigger,
    sinceBoardConnected,
    sinceStoredSnapshot;

uint16_t remote_battery_percent = 0;
bool remoteBattCharging = false;
bool display_task_initialised = false;
bool display_task_showing_option_screen = false;
int oldCounter = 0;

//------------------------------------------------------------

xQueueHandle xDisplayChangeEventQueue;
xQueueHandle xCommsStateEventQueue;
xQueueHandle xButtonPushEventQueue;

EventQueueManager *displayChangeQueueManager;
EventQueueManager *buttonQueueManager;

//------------------------------------------------------------------
enum DispStateEvent
{
  DISP_EV_NO_EVENT = 0,
  DISP_EV_CONNECTED,
  DISP_EV_DISCONNECTED,
  DISP_EV_STOPPED,
  DISP_EV_MOVING,
  DISP_EV_SW_RESET,
  DISP_EV_UPDATE,
  DISP_EV_PRIMARY_SINGLE_CLICK,
  DISP_EV_PRIMARY_DOUBLE_CLICK,
  DISP_EV_PRIMARY_TRIPLE_CLICK
};

// displayState - prototypes
void send_to_display_event_queue(DispStateEvent ev);
void sendToBoard();

//------------------------------------------------------------------

#include <Storage.h>
#include <stats.h>

Stats stats;

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

void resetsAcknowledged_callback()
{
  storeInMemory<uint16_t>(STORE_STATS_SOFT_RSTS, 0);
}

void storeTimeMovingInMemory()
{
  storeInMemory<ulong>(STORE_STATS_TRIP_TIME, stats.timeMovingMS);
}

#include <throttle.h>

ThrottleClass throttle;

//---------------------------------------------------------------

#include <Button2.h>

#include <utils.h>
#include <screens.h>
#include <displayState.h>

#include <display_task_0.h>
#include <comms_connected_state.h>
#include <features/battery_measure.h>

#include <nrf_comms.h>

#include <peripherals.h>

//---------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

  // core0Apm = new APM("core0 APM");

  statsStore.begin(STORE_STATS, /*read-only*/ false);

  // get the number of resets
  stats.soft_resets = statsStore.getUInt(STORE_STATS_SOFT_RSTS, 0);

  stats.setResetReasons(rtc_get_reset_reason(0), rtc_get_reset_reason(1));
  stats.setResetsAcknowledgedCallback(resetsAcknowledged_callback);

  configStore.begin(STORE_CONFIG, false);

  if (stats.watchdogReset())
  {
    stats.soft_resets++;
    // statsStore.putUInt(STORE_STATS_SOFT_RSTS, stats.soft_resets);
    storeInMemory<uint16_t>(STORE_STATS_SOFT_RSTS, stats.soft_resets);
    stats.timeMovingMS = readFromMemory<ulong>(STORE_STATS_TRIP_TIME);
#ifdef PRINT_RESET_DETECTION
    Serial.printf("RESET!!! =========> %d (%ums)\n", stats.soft_resets, stats.timeMovingMS);
#endif
  }
  else if (stats.powerOnReset())
  {
    stats.soft_resets = 0;
    statsStore.putUInt(STORE_STATS_SOFT_RSTS, stats.soft_resets);
  }
  statsStore.end();

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packetAvailable_cb);

  print_build_status(chipId);

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
  xButtonPushEventQueue = xQueueCreate(3, sizeof(uint8_t));

  displayChangeQueueManager = new EventQueueManager(xDisplayChangeEventQueue, 5);
  buttonQueueManager = new EventQueueManager(xButtonPushEventQueue, 10);

  while (!display_task_initialised)
  {
    vTaskDelay(1);
  }
}
//---------------------------------------------------------------

elapsedMillis
    since_sent_config_to_board,
    sinceNRFUpdate,
    sinceLastSwReset;
uint8_t old_throttle;
ulong loopNum = 0;

void loop()
{
  if (sinceSentToBoard > SEND_TO_BOARD_INTERVAL)
  {
    // if (core0Apm->running())
    //   core0Apm->stop();

    // core0Apm->start(loopNum == 20);
    loopNum++;

    // core0Apm->addPoint(PointState::ON, "sendBoard()");
    sendToBoard();
    // core0Apm->addPoint(PointState::OFF, "");
  }

  if (board.hasTimedout())
  {
    sendToCommsEventStateQueue(EV_COMMS_BOARD_TIMEDOUT);
  }

  if (sinceNRFUpdate > 20)
  {
    sinceNRFUpdate = 0;
    // core0Apm->addPoint(PointState::ON, "nrf24.update()");
    nrf24.update();
    // core0Apm->addPoint(PointState::OFF, "");
  }

  primaryButton.loop();

  vTaskDelay(1);
}

//------------------------------------------------------------------

void sendToBoard()
{

  bool accelEnabled =
      throttle.get() < 127 || // braking
      board.packet.moving ||
      !featureService.get<bool>(PUSH_TO_START) ||
      (featureService.get<bool>(PUSH_TO_START) && primaryButton.isPressed());

  bool cruiseControlActive =
      board.packet.moving &&
      FEATURE_CRUISE_CONTROL &&
      primaryButton.isPressed();

  sinceSentToBoard = 0;
  controller_packet.throttle = throttle.get(/*enabled*/ accelEnabled);
  controller_packet.cruise_control = cruiseControlActive;
  sendPacketToBoard();
}
//------------------------------------------------------------------

#endif