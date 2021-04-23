#include <Arduino.h>
#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <tasks/queues/types/root.h>

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

#include <types.h>
#include <rtosManager.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>
#include <Wire.h>

#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <rom/rtc.h> // for reset reason
#include <shared-utils.h>
#include <types.h>

// TASKS ------------------------

#include <tasks/root.h>
#include <tasks/queues/Managers.h>
// #include <testUtils.h>

// #include <Fsm.h>
// #include <FsmManager.h>

// #include <constants.h>

// Comms::Event ev = Comms::Event::BOARD_FIRST_PACKET;

// #include <RF24Network.h>
// #include <NRF24L01Lib.h>
// #include <GenericClient.h>

// #define RADIO_OBJECTS
// NRF24L01Lib nrf24;

// RF24 radio(NRF_CE, NRF_CS);
// RF24Network network(radio);
// GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

// #include <Preferences.h>
// #include <BatteryLib.h>
// #include <QueueManager.h>
// #include <RTOSTaskManager.h>

//------------------------------------------------------------
// #include "rtosManager.h"

// xQueueHandle xBoardPacketQueue;
// Queue::Manager *boardPacketQueue;

// xQueueHandle xStatsQueue;
// Queue::Manager *statsQueue;

// xQueueHandle xPerihperals;
// Queue::Manager *peripheralsQueue;

// xQueueHandle xPrimaryButtonQueueHandle;
// Queue::Manager *primaryButtonQueue = nullptr;

// MyMutex mutex_I2C;
// MyMutex mutex_SPI;

//------------------------------------------------------------

// ControllerData controller_packet;
// ControllerConfig controller_config;

// #include <BoardClass.h>

// BoardClass board;

// nsPeripherals::Peripherals *peripherals;

//------------------------------------------------------------------

// #include <FeatureService.h>

//------------------------------------------------------------------

// prototypes
// void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
// void waitForTasksToBeReady();

// void boardConnectedState_cb();
// void printSentToBoard_cb(ControllerData data);
// void printRecvFromBoard_cb(VescData data);

// void boardClientInit()
// {
//   boardClient.begin(&network, boardPacketAvailable_cb, mutex_SPI.handle());
//   boardClient.setConnectedStateChangeCallback(boardConnectedState_cb);
//   boardClient.setSentPacketCallback(printSentToBoard_cb);
//   boardClient.setReadPacketCallback(printRecvFromBoard_cb);
// }

// #ifdef COMMS_M5ATOM
// GenericClient<uint16_t, uint16_t> m5AtomClient(COMMS_M5ATOM);
// void m5AtomClientInit()
// {
//   m5AtomClient.begin(&network, [](uint16_t from, uint8_t type) {
//     uint16_t packet = m5AtomClient.read();
//     Serial.printf("rx %d from M5Atom!\n", packet);
//     m5AtomClient.sendTo(0, packet);
//   });
// }
// #endif

//------------------------------------------------------------------

#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
#endif
//------------------------------------------------------------------

// elapsedMillis
//     sinceSentToBoard,
//     sinceLastBoardPacketRx,
//     sinceSentRequest,
//     since_read_trigger,
//     sinceBoardConnected,
//     sinceStoredSnapshot;

// int oldCounter = 0;

// prototypes
// void sendToBoard();

//------------------------------------------------------------------

// #include <tasks/core0/statsTask.h>
// #include <tasks/core0/remoteTask.h>
#include <utils.h>

// #include <tasks/core0/NintendoClassicTask.h>

// #include <tasks/core0/ThrottleTask.h>

// #if (USING_DISPLAY == 1)
// #include <displayState.h>
// #include <tasks/core0/DisplayTask.h>
// #endif

// #if (USING_LED == 1)
// #include <tasks/core0/ledTask.h>
// #endif

// #if (USING_DEBUG_TASK == 1)
// #include <tasks/core0/debugTask.h>
// #endif

// #include <tasks/core0/commsStateTask.h>
// #include <nrf_comms.h>

// #if (USING_QWIIC_BUTTON_TASK == 1)
// #include <tasks/core0/QwiicButtonTask.h>
// #endif

// #include <peripherals.h>
// #include <tasks/core0/peripheralsTask.h>

// #include <assert.h>
// #define __ASSERT_USE_STDERR

void createQueues();
void createQueueManagers();
void startTasks();
void configureTasks();
void waitForTasks();
void enableTasks(bool print = false);

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

  Wire.begin();

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

#ifdef RELEASE_BUILD
  print_build_status(chipId);
#endif
  vTaskDelay(100);

  startTasks();

  configureTasks();

  waitForTasks();

  enableTasks();
}
//---------------------------------------------------------------

void loop()
{

  vTaskDelay(TICKS_10ms);
}

//------------------------------------------------------------------

void startTasks()
{
  BoardCommsTask::start(TASK_PRIORITY_4, /*work*/ PERIOD_100ms, /*send*/ PERIOD_200ms);
  DisplayTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms);
  NintendoClassicTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms);
  QwiicTaskBase::start(TASK_PRIORITY_2, /*work*/ PERIOD_100ms);
  ThrottleTaskBase::start(TASK_PRIORITY_4, /*work*/ PERIOD_200ms);
}

void configureTasks()
{
}

void waitForTasks()
{
  while (
      BoardCommsTask::thisTask->ready == false ||
      DisplayTaskBase::thisTask->ready == false ||
      NintendoClassicTaskBase::thisTask->ready == false ||
      QwiicTaskBase::thisTask->ready == false ||
      ThrottleTaskBase::thisTask->ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);
}

void enableTasks(bool print)
{
  BoardCommsTask::thisTask->enable(print);
  DisplayTaskBase::thisTask->enable(print);
  NintendoClassicTaskBase::thisTask->enable(print);
  QwiicTaskBase::thisTask->enable(print);
  ThrottleTaskBase::thisTask->enable(print);
}

#endif // UNIT_TEST