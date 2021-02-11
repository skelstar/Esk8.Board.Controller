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
#include <QueueManager.h>

//------------------------------------------------------------
#include "rtosManager.h"

xQueueHandle xDisplayEventQueue;
xQueueHandle xButtonPushEventQueue;

Queue::Manager *displayQueue;
Queue::Manager *buttonQueue;

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

namespace Board
{
  MyMutex mutex1;

  void init()
  {
    mutex1.create("board", TICKS_2);
    // mutex1.enabled = false;
  }
} // namespace Board

// prototypes
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

void printSentToBoard_cb(ControllerData data)
{
  if (PRINT_TX_TO_BOARD)
    Serial.printf(TX_TO_BOARD_FORMAT, (int)data.id);
}
void printRecvFromBoard_cb(VescData data)
{
  if (PRINT_RX_FROM_BOARD)
    Serial.printf(RX_FROM_BOARD_FORMAT, data.id);
}

GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);
void boardClientInit()
{
  boardClient.begin(&network, boardPacketAvailable_cb);
  boardClient.setConnectedStateChangeCallback([] {
    if (PRINT_BOARD_CLIENT_CONNECTED_CHANGED)
      Serial.printf(BOARD_CLIENT_CONNECTED_FORMAT, boardClient.connected() ? "CONNECTED" : "DISCONNECTED");
  });
  boardClient.setSentPacketCallback(printSentToBoard_cb);
  boardClient.setReadPacketCallback(printRecvFromBoard_cb);
}

#ifdef COMMS_M5ATOM
GenericClient<uint16_t, uint16_t> m5AtomClient(COMMS_M5ATOM);
void m5AtomClientInit()
{
  m5AtomClient.begin(&network, [](uint16_t from, uint8_t type) {
    uint16_t packet = m5AtomClient.read();
    Serial.printf("rx %d from M5Atom!\n", packet);
    m5AtomClient.sendTo(0, packet);
  });
}
#endif

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
    sinceSentRequest,
    since_read_trigger,
    sinceBoardConnected,
    sinceStoredSnapshot;

uint16_t remote_battery_percent = 0;
bool remoteBattCharging = false;
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

#include <throttle.h>

ThrottleClass throttle;

//---------------------------------------------------------------

#include <tasks/core0/statsTask.h>
#include <utils.h>
#include <screens.h>

#include <displayState.h>

#include <tasks/core0/displayTask.h>
#include <tasks/core0/commsStateTask.h>
#include <nrf_comms.h>

#include <features/battery_measure.h>

#include <peripherals.h>
#include <assert.h>
#define __ASSERT_USE_STDERR

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

  Board::init();
  Stats::init();

  configStore.begin(STORE_CONFIG, false);

  if (Stats::mutex.take("setup()"))
  {
    if (stats.wasWatchdogReset())
    {
      stats.controllerResets++;
      storeInMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS, stats.controllerResets);
      stats.timeMovingMS = readFromMemory<ulong>(STORE_STATS, STORE_STATS_TRIP_TIME);
      if (PRINT_RESET_DETECTION)
        Serial.printf("RESET!!! =========> controllerResets: %d\n", stats.controllerResets);
    }
    else if (stats.powerOnReset())
    {
      stats.controllerResets = 0;
      storeInMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS, stats.controllerResets);
    }
    Stats::mutex.give("setup()");
  }

  nrf24.begin(&radio, &network, COMMS_CONTROLLER);

  boardClientInit();
#ifdef COMMS_M5ATOM
  m5AtomClientInit();
#endif

  print_build_status(chipId);

  throttle.init(/*pin*/ 27, [](uint8_t throttle) {
    // Serial.printf("throttle changed: %d (cruise: %d, pressed: %d)\n",
    //               throttle,
    //               controller_packet.cruise_control,
    //               primaryButton.isPressedRaw());
  });

  primaryButtonInit();
  rightButtonInit();

  vTaskDelay(100);

  // CORE_0
  Display::createTask(DISPLAY_TASK_CORE, TASK_PRIORITY_3);
  Comms::createTask(COMMS_TASK_CORE, TASK_PRIORITY_2);
  Battery::createTask(BATTERY_TASK_CORE, TASK_PRIORITY_1);
  Stats::createTask(STATS_TASK_CORE, TASK_PRIORITY_1);

  // CORE_1
  xDisplayEventQueue = xQueueCreate(5, sizeof(uint8_t));
  xButtonPushEventQueue = xQueueCreate(3, sizeof(uint8_t));

  displayQueue = new Queue::Manager(xDisplayEventQueue, 5);
  displayQueue->setReadEventCallback(Display::queueReadCb);
  buttonQueue = new Queue::Manager(xButtonPushEventQueue, 10);

  while (!Display::taskReady &&
         !Comms::taskReady &&
         !Stats::taskReady &&
         !Battery::taskReady)
  {
    vTaskDelay(10);
  }

  sendConfigToBoard();
}
//---------------------------------------------------------------

elapsedMillis sinceNRFUpdate;

void loop()
{
  if (sinceSentToBoard > SEND_TO_BOARD_INTERVAL)
  {
    sendToBoard();
  }

  if (Board::mutex1.take(__func__))
  {
    if (board.hasTimedout())
      Comms::queue1->send(Comms::Event::BOARD_TIMEDOUT);
    Board::mutex1.give(__func__);
  }

  if (sinceNRFUpdate > 20)
  {
    sinceNRFUpdate = 0;
    boardClient.update();
#ifdef COMMS_M5ATOM
    m5AtomClient.update();
#endif
  }

  primaryButton.loop();
  rightButton.loop();

  vTaskDelay(1);
}

//------------------------------------------------------------------

void sendToBoard()
{
  bool throttleEnabled = false;
  bool cruiseControlActive = false;

  if (Board::mutex1.take(__func__, 50))
  {
    throttleEnabled =
        throttle.get() < 127 || // braking
        board.packet.moving ||
        !featureService.get<bool>(PUSH_TO_START) ||
        (featureService.get<bool>(PUSH_TO_START) && primaryButton.isPressedRaw());

    cruiseControlActive =
        board.packet.moving &&
        FEATURE_CRUISE_CONTROL &&
        primaryButton.isPressedRaw();
    Board::mutex1.give(__func__);
  }

  sinceSentToBoard = 0;
  controller_packet.throttle = throttle.get(/*enabled*/ throttleEnabled);
  controller_packet.cruise_control = cruiseControlActive;
  sendPacketToBoard();
}
//------------------------------------------------------------------

#endif