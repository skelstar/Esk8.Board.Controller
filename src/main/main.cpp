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
#include <shared-utils.h>
#include <types.h>
#include <printFormatStrings.h>
#include <Button2.h>

#include <constants.h>

Comms::Event ev = Comms::Event::BOARD_FIRST_PACKET;

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>

#include <TFT_eSPI.h>
#include <Preferences.h>
#include <BatteryLib.h>
#include <HUDData.h>
#include <QueueManager.h>

//------------------------------------------------------------

xQueueHandle xDisplayEventQueue;
xQueueHandle xButtonPushEventQueue;
xQueueHandle xHUDActionQueue;

Queue::Manager *displayQueue;
Queue::Manager *buttonQueue;
Queue::Manager *hudActionQueue;

Queue::Manager *nrfCommsQueue;
namespace NRFCommsQueue
{
  void init()
  {
    nrfCommsQueue = new Queue::Manager(/*length*/ 5, sizeof(uint8_t), /*ticks*/ 10);
    nrfCommsQueue->setName("nrfCommsQueue");
    nrfCommsQueue->setSentEventCallback([](uint16_t ev) {
      // if (ev != 1)
      //   Serial.printf("nrfCommsQueue sent\n", ev);
    });
  }
} // namespace NRFCommsQueue

Queue::Manager *hudTasksQueue;
namespace HudTaskQueue
{
  void queueSentEventCb(uint16_t ev)
  {
    if (PRINT_HUD_TASKS_QUEUE_SEND)
      Serial.printf(PRINT_QUEUE_SEND_FORMAT, HUDTask::getName(ev), "HUD_TASK");
  }
  void queueReadEventCb(uint16_t ev)
  {
    if (PRINT_HUD_TASKS_QUEUE_READ)
      Serial.printf(PRINT_QUEUE_READ_FORMAT, "HUD_TASK", HUDTask::getName(ev));
  }

  void init()
  {
    hudTasksQueue = new Queue::Manager(/*length*/ 3, sizeof(HUDTask::Message), /*ticks*/ 5);
    hudTasksQueue->setName("hudQueue");
    hudTasksQueue->setSentEventCallback(queueSentEventCb);
    hudTasksQueue->setReadEventCallback(queueReadEventCb);
  }
} // namespace HudTaskQueue

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

// prototypes
void hudPacketAvailable_cb(uint16_t from_id, uint8_t type);
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

GenericClient<HUD::Command, HUDAction::Event> hudClient(COMMS_HUD);
void hudClientInit()
{
  hudClient.begin(&network, hudPacketAvailable_cb);
  hudClient.setConnectedStateChangeCallback([] {
    Serial.printf(PRINT_CLIENT_CONNECTION_FORMAT, "HUD", hudClient.connected() ? "CONNECTED" : "DISCONNECTED");
  });
  hudClient.setSentPacketCallback([](HUD::Command command) {
    if (PRINT_TX_TO_HUD)
      Serial.printf(PRINT_TX_PACKET_TO_FORMAT, "HUD", command.getCommand());
  });
  hudClient.setReadPacketCallback([](HUDAction::Event ev) {
    if (PRINT_RX_FROM_HUD)
      Serial.printf(PRINT_RX_PACKET_FROM_FORMAT, "HUD", HUDAction::getName(ev));
  });
}

void printSentToBoard(ControllerData data)
{
  if (PRINT_TX_TO_BOARD)
    Serial.printf(TX_TO_BOARD_FORMAT, data.id);
}
void printRecvFromBoard(VescData data)
{
  if (PRINT_RX_FROM_BOARD)
    Serial.printf(RX_FROM_BOARD_FORMAT, data.id);
}

GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);
void boardClientInit()
{
  boardClient.begin(&network, boardPacketAvailable_cb);
  boardClient.setConnectedStateChangeCallback([] {
    Serial.printf(BOARD_CLIENT_CONNECTED_FORMAT, boardClient.connected() ? "CONNECTED" : "DISCONNECTED");
  });
  boardClient.setSentPacketCallback(printSentToBoard);
  boardClient.setReadPacketCallback(printRecvFromBoard);
}

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

class Config
{
public:
  uint8_t headlightMode;
};

Config config;

#define STORE_CONFIG "config"
#define STORE_CONFIG_ACCEL_COUNTS "accel counts"
#define STORE_CONFIG_BRAKE_COUNTS "brake counts"
Preferences configStore;

void storeTimeMovingInMemory()
{
  storeInMemory<ulong>(STORE_STATS_TRIP_TIME, stats.timeMovingMS);
}

#include <throttle.h>

ThrottleClass throttle;

//---------------------------------------------------------------

#include <utils.h>
#include <screens.h>

#include <displayState.h>

#include <nrf_comms.h>

#include <tasks/core0/displayTask.h>
#include <tasks/core0/commsStateTask.h>
#include <tasks/core1/hudTask.h>

#include <features/battery_measure.h>

#include <peripherals.h>
#include <assert.h>
#define __ASSERT_USE_STDERR

//------------------------------------------------------------------
void resetsAcknowledged_callback()
{
  using namespace HUD;
  storeInMemory<uint16_t>(STORE_STATS_SOFT_RSTS, 0);
  sendCommandToHud(FLASH | FAST | GREEN);
}
//------------------------------------------------------------------

void setup()
{

  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

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
    storeInMemory<uint16_t>(STORE_STATS_SOFT_RSTS, stats.soft_resets);
    stats.timeMovingMS = readFromMemory<ulong>(STORE_STATS_TRIP_TIME);
    if (PRINT_RESET_DETECTION)
      Serial.printf("RESET!!! =========> %d (%ums)\n", stats.soft_resets, stats.timeMovingMS);
  }
  else if (stats.powerOnReset())
  {
    stats.soft_resets = 0;
    statsStore.putUInt(STORE_STATS_SOFT_RSTS, stats.soft_resets);
  }
  statsStore.end();

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, boardPacketAvailable_cb);

  hudClientInit();
  boardClientInit();

  print_build_status(chipId);

  throttle.init(/*pin*/ 27);

  primaryButtonInit();

  vTaskDelay(100);

  // CORE_0
  Display::createTask(CORE_0, TASK_PRIORITY_3);
  Comms::createTask(CORE_0, TASK_PRIORITY_1);
  Battery::createTask("Battery Measure Task", CORE_0, TASK_PRIORITY_1);

  // CORE_1
  HUD::createTask(CORE_1, TASK_PRIORITY_1);

  xDisplayEventQueue = xQueueCreate(5, sizeof(uint8_t));
  xButtonPushEventQueue = xQueueCreate(3, sizeof(uint8_t));
  xHUDActionQueue = xQueueCreate(/*len*/ 3, sizeof(uint8_t));

  displayQueue = new Queue::Manager(xDisplayEventQueue, 5);
  buttonQueue = new Queue::Manager(xButtonPushEventQueue, 10);
  hudActionQueue = new Queue::Manager(xHUDActionQueue, 3, NO_MESSAGE_ON_QUEUE);

  NRFCommsQueue::init();
  HudTaskQueue::init();

  // force value to get first packet out

  using namespace HUD;

  while (!Display::taskReady && !Comms::taskReady && !HUD::taskReady)
  {
    vTaskDelay(1);
  }

  sendCommandToHud(HEARTBEAT);
  vTaskDelay(100);
  sendCommandToHud(TWO_FLASHES | BLUE | FAST);

  sendConfigToBoard();
}
//---------------------------------------------------------------

elapsedMillis sinceNRFUpdate, sinceSentToHudTest;

void loop()
{
  if (sinceSentToBoard > SEND_TO_BOARD_INTERVAL)
  {
    sendToBoard();
  }

  if (board.hasTimedout())
  {
    nrfCommsQueue->send(Comms::Event::BOARD_TIMEDOUT);
  }

  if (sinceNRFUpdate > 20)
  {
    sinceNRFUpdate = 0;
    hudClient.update();
    boardClient.update();
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