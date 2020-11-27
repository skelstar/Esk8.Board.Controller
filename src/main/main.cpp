#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

// #include <Arduino_Helpers.h>
// #include <AH/Debug/Debug.hpp>

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <rom/rtc.h> // for reset reason
#include <shared-utils.h>

// used in TFT_eSPI library as alternate SPI port (HSPI?)
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define NRF_CE 17
#define NRF_CS 2

#include "$PROJECT_HASH/../types.h"
#include <constants.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <TFT_eSPI.h>
#include <Preferences.h>
#include <BatteryLib.h>
#include <HUDData.h>
#include <EventQueueManager.h>
//------------------------------------------------------------

xQueueHandle xDisplayChangeEventQueue;
xQueueHandle xCommsStateEventQueue;
xQueueHandle xButtonPushEventQueue;
xQueueHandle xHUDMessageEventQueue;
xQueueHandle xHUDActionQueue;
xQueueHandle xTaskActionEventQueue;

EventQueueManager *displayChangeQueueManager;
EventQueueManager *nrfCommsQueueManager;
EventQueueManager *buttonQueue;
EventQueueManager *hudMessageQueueManager;
EventQueueManager *hudActionQueueManager;
EventQueueManager *taskQueueManager;

xTaskHandle hudTaskHandle;

//------------------------------------------------------------
enum FeatureType
{
  CRUISE_CONTROL,
  PUSH_TO_START
};

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

ControllerData controller_packet;
ControllerConfig controller_config;

#include <BoardClass.h>

BoardClass board;

//------------------------------------------------------------------

Preferences statsStore;

//------------------------------------------------------------------

#include <Storage.h>

#include <stats.h>

Stats stats;

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

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library

//------------------------------------------------------------------

elapsedMillis
    sinceSentToBoard,
    sinceLastBoardPacketRx,
    sinceLastHudPacket,
    sinceSentRequest,
    since_read_trigger,
    sinceBoardConnected,
    sinceStoredSnapshot;

uint16_t remote_battery_percent = 0;
bool remoteBattCharging = false;
bool display_task_initialised = false;
bool display_task_showing_option_screen = false;
int oldCounter = 0;

// prototypes
void sendToBoard();

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

HUDData hudData;

//---------------------------------------------------------------

#include <Button2.h>

#include <utils.h>
#include <screens.h>
#include <displayState.h>

#include <nrf_comms.h>
#include <tasks/display_task_0.h>
#include <tasks/comms_connected_state.h>
#include <tasks/hudTask.h>
#include <features/battery_measure.h>

#include <peripherals.h>
#include <assert.h>
#define __ASSERT_USE_STDERR
//------------------------------------------------------------------

void asserts()
{
  if (DispStateEvent::DISP_EV_Length != ARRAY_SIZE(dispStateEventNames))
  {
    Serial.printf("DispStateEvent has more elements than names (%d %d)\n", DispStateEvent::DISP_EV_Length, ARRAY_SIZE(dispStateEventNames));
    assert(false);
  }
  if (HudActionEvent::HUD_ACTION_Length != ARRAY_SIZE(hudActionEventNames))
  {
    Serial.printf("HudActionEvent has more elements than names (%d %d)\n", HudActionEvent::HUD_ACTION_Length, ARRAY_SIZE(hudActionEventNames));
    assert(false);
  }
  if (CommsStateEvent::EV_COMMS_Length != ARRAY_SIZE(commsStateEventNames))
  {
    Serial.printf("CommsStateEvent has more elements than names (%d %d)\n", CommsStateEvent::EV_COMMS_Length, ARRAY_SIZE(commsStateEventNames));
    assert(false);
  }
}

void setup()
{

  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

  asserts();

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

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

  // CORE_0
  createDisplayTask0(CORE_0, TASK_PRIORITY_3);
  createCommsStateTask_0(CORE_0, TASK_PRIORITY_1);
  createBatteryMeasureTask(CORE_0, TASK_PRIORITY_1);

  // CORE_1
  createHudTask(CORE_1, TASK_PRIORITY_1);

  xDisplayChangeEventQueue = xQueueCreate(5, sizeof(uint8_t));
  xCommsStateEventQueue = xQueueCreate(5, sizeof(uint8_t));
  xButtonPushEventQueue = xQueueCreate(3, sizeof(uint8_t));
  xHUDMessageEventQueue = xQueueCreate(3, sizeof(uint8_t));
  xHUDActionQueue = xQueueCreate(/*len*/ 3, sizeof(uint8_t));
  xTaskActionEventQueue = xQueueCreate(/*len*/ 3, sizeof(uint8_t));

  displayChangeQueueManager = new EventQueueManager(xDisplayChangeEventQueue, 5);
  nrfCommsQueueManager = new EventQueueManager(xCommsStateEventQueue, 5);
  buttonQueue = new EventQueueManager(xButtonPushEventQueue, 10);
  hudMessageQueueManager = new EventQueueManager(xHUDMessageEventQueue, 10);
  hudActionQueueManager = new EventQueueManager(xHUDActionQueue, 3);
  taskQueueManager = new EventQueueManager(xTaskActionEventQueue, 5);

  while (!display_task_initialised)
  {
    vTaskDelay(1);
  }
}
//---------------------------------------------------------------

elapsedMillis
    since_sent_config_to_board,
    sinceNRFUpdate,
    sinceSentToHud,
    sinceSendTestCommand,
    sinceLastSwReset;
uint8_t old_throttle;

HUDCommand hudCommands[] = {
    HUDCommand::HUD_CMD_IDLE,
    HUDCommand::HUD_CMD_FLASH_GREEN,
    HUDCommand::HUD_CMD_PULSE_RED,
    HUDCommand::HUD_CMD_IDLE,
    HUDCommand::HUD_CMD_SPIN_GREEN,
    HUDCommand::HUD_CMD_IDLE,
};

uint8_t hudCommandIdx = 0;

void loop()
{
  if (sinceSentToBoard > SEND_TO_BOARD_INTERVAL)
  {
    sendToBoard();
  }

  if (board.hasTimedout())
  {
    nrfCommsQueueManager->send(EV_COMMS_BOARD_TIMEDOUT);
  }

  if (sinceSentToHud > 1000)
  {
    sinceSentToHud = 0;
    bool ok = sendPacketToHud(HUD_CMD_HEARTBEAT, stats.hudConnected == false); // print of not connected
    stats.updateHud(ok);

    if (stats.hudConnected && sinceSendTestCommand > 3000)
    {
      sinceSendTestCommand = 0;
      ok = sendPacketToHud(hudCommands[hudCommandIdx++], true);
      if (hudCommandIdx == 6)
        hudCommandIdx = 0;
    }
    // bool ok = sendPacketToHud(HUD_CMD_HEARTBEAT, stats.hudConnected == false); // print of not connected

    // if (ok && stats.hudConnected == false)
    // {
    //   stats.hudConnected = true;
    //   sendPacketToHud(HUD_CMD_FLASH_GREEN, true);
    // }
    // else if (!ok && stats.hudConnected)
    // {
    //   stats.hudConnected = false;
    // }
  }

  if (sinceNRFUpdate > 20)
  {
    sinceNRFUpdate = 0;
    nrf24.update();
  }

  primaryButton.loop();

  vTaskDelay(1);
}

//------------------------------------------------------------------

void sendToBoard()
{

  bool throttleEnabled =
      throttle.get() < 127 || // braking
      board.packet.moving ||
      !featureService.get<bool>(PUSH_TO_START) ||
      (featureService.get<bool>(PUSH_TO_START) && primaryButton.isPressed());

  bool cruiseControlActive =
      board.packet.moving &&
      FEATURE_CRUISE_CONTROL &&
      primaryButton.isPressed();

  sinceSentToBoard = 0;
  controller_packet.throttle = throttle.get(/*enabled*/ throttleEnabled);
  controller_packet.cruise_control = cruiseControlActive;
  sendPacketToBoard();
}
//------------------------------------------------------------------

#endif