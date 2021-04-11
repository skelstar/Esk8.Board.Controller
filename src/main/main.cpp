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

#include <Fsm.h>
#include <FsmManager.h>

#include <constants.h>

Comms::Event ev = Comms::Event::BOARD_FIRST_PACKET;

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>

#include <Preferences.h>
#include <BatteryLib.h>
#include <QueueManager.h>
#include <RTOSTaskManager.h>

//------------------------------------------------------------
#include "rtosManager.h"

xQueueHandle xBoardPacketQueue;
Queue::Manager *boardPacketQueue;

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue;

xQueueHandle xPerihperals;
Queue::Manager *peripheralsQueue;

xQueueHandle xPrimaryButtonQueueHandle;
Queue::Manager *primaryButtonQueue = nullptr;

MyMutex mutex_I2C;
MyMutex mutex_SPI;

//------------------------------------------------------------

ControllerData controller_packet;
ControllerConfig controller_config;

#include <BoardClass.h>

BoardClass board;

nsPeripherals::Peripherals *peripherals;

//------------------------------------------------------------------

#include <FeatureService.h>

//------------------------------------------------------------------

// prototypes
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
void waitForTasksToBeReady();

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

void boardConnectedState_cb();
void printSentToBoard_cb(ControllerData data);
void printRecvFromBoard_cb(VescData data);

void boardClientInit()
{
  boardClient.begin(&network, boardPacketAvailable_cb, mutex_SPI.handle());
  boardClient.setConnectedStateChangeCallback(boardConnectedState_cb);
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

#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
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

#include <tasks/core0/statsTask.h>
#include <tasks/core0/remoteTask.h>
#include <utils.h>

#include <tasks/core0/NintendoClassicTask.h>

#include <tasks/core0/ThrottleTask.h>

#if (USING_DISPLAY == 1)
#include <displayState.h>
#include <tasks/core0/displayTask.h>
#endif

#if (USING_LED == 1)
#include <tasks/core0/ledTask.h>
#endif

#if (USING_DEBUG_TASK == 1)
#include <tasks/core0/debugTask.h>
#endif

#include <tasks/core0/commsStateTask.h>
#include <nrf_comms.h>

#if (USING_QWIIC_BUTTON_TASK == 1)
#include <tasks/core0/QwiicButtonTask.h>
#endif

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

  Stats::init();

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

#ifdef COMMS_M5ATOM
  m5AtomClientInit();
#endif

  print_build_status(chipId);

  vTaskDelay(100);

  // CORE_0
  Comms::mgr.create(Comms::task, CORE_0, TASK_PRIORITY_2);
#if (USING_DISPLAY == 1)
  Display::mgr.create(Display::task, CORE_0, TASK_PRIORITY_1);
#endif
#if (USING_REMOTE == 1)
  Remote::mgr.create(Remote::task, CORE_0, TASK_PRIORITY_1);
#endif
#if (USING_STATS == 1)
  Stats::createTask(STATS_TASK_CORE, TASK_PRIORITY_1);
#endif
#if (USING_LED == 1)
  Led::createTask(LED_TASK_CORE, TASK_PRIORITY_1);
#endif
#if (USING_DEBUG_TASK == 1)
  Debug::mgr.create(Debug::task, CORE_0, TASK_PRIORITY_0);
#endif
#if (USING_QWIIC_BUTTON_TASK == 1)
  QwiicButtonTask::mgr.create(QwiicButtonTask::task, CORE_0, TASK_PRIORITY_2);
#endif
  // ThrottleTask::createTask(0, TASK_PRIORITY_1);
  ThrottleTask::mgr.create(ThrottleTask::task, CORE_0, TASK_PRIORITY_1); //(0, TASK_PRIORITY_1);
#if (USING_NINTENDO_BUTTONS == 1)
  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, TASK_PRIORITY_0);
#endif

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  boardPacketQueue = new Queue::Manager(xBoardPacketQueue, TICKS_5ms);

  xStatsQueue = xQueueCreate(3, sizeof(StatsClass *));
  statsQueue = new Queue::Manager(xStatsQueue, TICKS_5ms);

  xPerihperals = xQueueCreate(1, sizeof(nsPeripherals::Peripherals *));
  peripheralsQueue = new Queue::Manager(xPerihperals, TICKS_5ms);

  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState));
  primaryButtonQueue = new Queue::Manager(xPrimaryButtonQueueHandle, TICKS_5ms);

  peripherals = new nsPeripherals::Peripherals();

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  mutex_SPI.create("SPI", /*default*/ TICKS_50ms);
  mutex_SPI.enabled = true;

  boardClientInit();

  waitForTasksToBeReady();

  sendConfigToBoard();
}
//---------------------------------------------------------------

elapsedMillis sinceNRFUpdate, since_update_throttle;
uint8_t old_buttons[NintendoController::BUTTON_COUNT];
unsigned long last_throttle_id = 0;

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

  if (since_update_throttle > SEND_TO_BOARD_INTERVAL)
  {
    since_update_throttle = 0;

    ThrottleState *throttle = ThrottleTask::queue->peek<ThrottleState>("PeripheralsTask loop");
    if (throttle != nullptr && !throttle->been_peeked(last_throttle_id))
    {
      if (throttle->missed_packet(last_throttle_id))
        Serial.printf("[MAIN_LOOP] missed at least one throttle packet! (id: %lu, last: %lu)\n", throttle->event_id, last_throttle_id);

      controller_packet.throttle = throttle->val;
      last_throttle_id = throttle->event_id;
    }
  }

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
  braking = controller_packet.throttle < 127;
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

  controller_packet.cruise_control = cruiseControlActive;

  sinceSentToBoard = 0;
  sendPacketToBoard();
}
//------------------------------------------------------------------

void waitForTasksToBeReady()
{
  while (
      (USING_DISPLAY == 0 || !Display::mgr.ready) &&
      (USING_NINTENDO_BUTTONS == 0 || !NintendoClassicTask::mgr.ready) &&
      !Comms::mgr.ready &&
      !Stats::taskReady &&
      (REMOTE_TASK_CORE == -1 || !Remote::mgr.ready) &&
      (USING_QWIIC_BUTTON_TASK == 0 || !QwiicButtonTask::mgr.ready))
  {
    vTaskDelay(10);
  }
}

#endif