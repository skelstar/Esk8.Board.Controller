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
#include <NintendoController.h>

#include <constants.h>

Comms::Event ev = Comms::Event::BOARD_FIRST_PACKET;

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>

#if OPTION_USING_DISPLAY
#include <TFT_eSPI.h>
#endif

#include <Preferences.h>
#include <BatteryLib.h>
#include <QueueManager.h>

//------------------------------------------------------------
#include "rtosManager.h"

xQueueHandle xDisplayEventQueue;
Queue::Manager *displayQueue;

xQueueHandle xButtonPushEventQueue;
Queue::Manager *buttonQueue;

xQueueHandle xControllerPacketQueue;
Queue::Manager *controllerPacketQueue;

xQueueHandle xBoardPacketQueue;
Queue::Manager *boardPacketQueue;

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue;

xQueueHandle xPerihperals;
Queue::Manager *mgPeripherals;

//------------------------------------------------------------

ControllerData controller_packet;
ControllerConfig controller_config;

#include <BoardClass.h>

BoardClass board;

nsPeripherals::Peripherals *peripherals;

//------------------------------------------------------------------

#include <FeatureService.h>

//------------------------------------------------------------------

namespace Board
{
  MyMutex mutex;

  void init()
  {
    mutex.create("board", TICKS_2);
    // mutex.enabled = false;
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

//------------------------------------------------------------------

#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
#endif
//------------------------------------------------------------------

#if OPTION_USING_DISPLAY
TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library
#endif

//------------------------------------------------------------------

elapsedMillis
    sinceSentToBoard,
    sinceLastBoardPacketRx,
    sinceSentRequest,
    since_read_trigger,
    sinceBoardConnected,
    sinceStoredSnapshot;

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

// #include <throttle.h>

//---------------------------------------------------------------
// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

#if OPTION_USING_MAG_THROTTLE

#include <AS5600.h>

AMS_5600 ams5600;

namespace MagThrottle
{
  bool connect()
  {
    if (ams5600.detectMagnet() == 0)
    {
      Serial.printf("searching....\n");
      while (1)
      {
        if (ams5600.detectMagnet() == 1)
        {
          Serial.printf("Current Magnitude: %d\n", ams5600.getMagnitude());
          break;
        }
        else
        {
          Serial.println("Can not detect magnet");
        }
        delay(1000);
      }
    }
  }
} // namespace MagThrottle

#include <MagThrottle.h>

#include <NintendoController.h>
#include <NintendoButtons.h>
#include <SparkFun_Qwiic_Button.h>
//https://www.sparkfun.com/products/15932

#endif

//---------------------------------------------------------------

#include <tasks/core0/statsTask.h>
#include <tasks/core0/remoteTask.h>
#include <utils.h>

#if OPTION_USING_DISPLAY
#include <screens.h>
#include <displayState.h>
#include <tasks/core0/displayTask.h>
#endif
#if (FEATURE_LED_COUNT > 0)
#include <tasks/core0/ledTask.h>
#endif
#include <tasks/core0/commsStateTask.h>
#include <nrf_comms.h>

#include <peripherals.h>
#include <tasks/core0/peripheralsTask.h>

#include <assert.h>
#define __ASSERT_USE_STDERR

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

  Wire.begin();

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

  Board::init();
  Stats::init();

  configStore.begin(STORE_CONFIG, false);

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

  nrf24.begin(&radio, &network, COMMS_CONTROLLER);

  boardClientInit();
#ifdef COMMS_M5ATOM
  m5AtomClientInit();
#endif

  print_build_status(chipId);

  // #if (OPTION_USING_MAG_THROTTLE == 0)
  //   {
  //     throttle.init(/*pin*/ THROTTLE_PIN, [](uint8_t throttle) {
  //       Serial.printf("throttle changed: %d (cruise: %d)\n",
  //                     throttle,
  //                     controller_packet.cruise_control);
  //     });

  //     primaryButtonInit();
  //     rightButtonInit();
  //   }
  // #endif

  vTaskDelay(100);

// CORE_0
#if OPTION_USING_DISPLAY
  Display::createTask(DISPLAY_TASK_CORE, TASK_PRIORITY_3);
#endif
  Comms::createTask(COMMS_TASK_CORE, TASK_PRIORITY_2);
  Remote::createTask(BATTERY_TASK_CORE, TASK_PRIORITY_1);
  Stats::createTask(STATS_TASK_CORE, TASK_PRIORITY_1);
  nsPeripherals::createTask(PERIPHERALS_TASK_CORE, 3);

#if (FEATURE_LED_COUNT > 0)
  Led::createTask(LED_TASK_CORE, TASK_PRIORITY_1);
#endif

  // have to have regardless of whether we have a display or not
  // TODO fix this dependency
  xDisplayEventQueue = xQueueCreate(5, sizeof(uint8_t));
  displayQueue = new Queue::Manager(xDisplayEventQueue, 5);
#if OPTION_USING_DISPLAY
  displayQueue->setReadEventCallback(Display::queueReadCb);
#endif

  xButtonPushEventQueue = xQueueCreate(3, sizeof(uint8_t));
  buttonQueue = new Queue::Manager(xButtonPushEventQueue, 10);

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  boardPacketQueue = new Queue::Manager(xBoardPacketQueue, (TickType_t)5);

  xStatsQueue = xQueueCreate(3, sizeof(StatsClass *));
  statsQueue = new Queue::Manager(xStatsQueue, (TickType_t)5);

  xPerihperals = xQueueCreate(1, sizeof(nsPeripherals::Peripherals *));
  mgPeripherals = new Queue::Manager(xPerihperals, (TickType_t)5);

  bool displayReady = false;
  peripherals = new nsPeripherals::Peripherals();

  while (
#if OPTION_USING_DISPLAY
      !Display::taskReady &&
#endif
      !Comms::taskReady &&
      !Stats::taskReady &&
      !Remote::taskReady)
  {
    vTaskDelay(10);
  }

  sendConfigToBoard();
}
//---------------------------------------------------------------

elapsedMillis sinceNRFUpdate, since_update_throttle;

void loop()
{
  if (sinceSentToBoard > SEND_TO_BOARD_INTERVAL)
  {
    sendToBoard();
  }

  if (sinceNRFUpdate > 20)
  {
    sinceNRFUpdate = 0;
    boardClient.update();
#ifdef COMMS_M5ATOM
    m5AtomClient.update();
#endif
  }

#if OPTION_USING_MAG_THROTTLE
  if (since_update_throttle > SEND_TO_BOARD_INTERVAL)
  {
    since_update_throttle = 0;

    nsPeripherals::Peripherals *res = mgPeripherals->peek<nsPeripherals::Peripherals>();
    if (res != nullptr && res->throttle != peripherals->throttle)
    {
      peripherals = new nsPeripherals::Peripherals(*res);
      updateThrottle(peripherals);
    }
  }
#endif

  primaryButtonLoop();
  rightButton.loop();

  vTaskDelay(1);
}

//------------------------------------------------------------------

void sendToBoard()
{
  bool throttleEnabled = false;
  bool cruiseControlActive = false;
  bool primaryButtonPressed = false;
  bool braking = false;

#ifdef PRIMARY_BUTTON_PIN
  primaryButtonPressed = primaryButton.isPressedRaw();
  braking = throttle.get() < 127;

#endif
#if OPTION_USING_MAG_THROTTLE
  primaryButtonPressed = peripherals->primary_button == 1; // qwiicButton.isPressed();
  braking = MagThrottle::get() < 127;
#else

#endif

  throttleEnabled =
      braking || // braking
      board.packet.moving ||
      !featureService.get<bool>(FeatureType::PUSH_TO_START) ||
      (featureService.get<bool>(FeatureType::PUSH_TO_START) && primaryButtonPressed);

#if OPTION_USING_MAG_THROTTLE
  cruiseControlActive = false;
#else
  cruiseControlActive =
      board.packet.moving &&
      FEATURE_CRUISE_CONTROL &&
      primaryButtonPressed;
#endif

  sinceSentToBoard = 0;
  // controller_packet.throttle = throttle.get(/*enabled*/ throttleEnabled);
  controller_packet.throttle = MagThrottle::get();
  controller_packet.cruise_control = cruiseControlActive;
  sendPacketToBoard();
}
//------------------------------------------------------------------

#endif