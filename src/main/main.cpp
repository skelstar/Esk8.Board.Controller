#include <Arduino.h>
#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <BoardConfig.h>
#include <tasks/queues/queues.h>
#include <tasks/queues/types/root.h>

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

#include <types.h>
#include <rtosManager.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <Wire.h>

#include <constants.h>

#if REMOTE_USED == NINTENDO_REMOTE
#include <SparkFun_Qwiic_Button.h>
#include <MagThumbwheel.h>
#elif REMOTE_USED == RED_REMOTE
#include <Button2.h>
#include <AnalogThumbwheel.h>
#endif

#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>
RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);
NRF24L01Lib nrf24;

#include <rom/rtc.h> // for reset reason
#include <shared-utils.h>

// TASKS ------------------------

#include <tasks/root.h>

// LOCAL QUEUE MANAGERS -----------------------

Queue1::Manager<Transaction> *transactionQueue = nullptr;

//------------------------------------------------------------------

#include <utils.h>

void createQueues();
void createLocalQueueManagers();
void startTasks();
void initialiseTasks();
void configureTasks();
void waitForTasks();
void enableTasks(bool print = false);

//------------------------------------------------------------------

void setup()
{
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
#endif
  Serial.printf("------------------------ BOOT ------------------------\n");

  Wire.begin();

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

#ifdef RELEASE_BUILD
  print_build_status(chipId);
#endif
  vTaskDelay(100);

  createQueues();

  createLocalQueueManagers();

  configureTasks();

  startTasks();

  initialiseTasks();

  waitForTasks();

  enableTasks(PRINT_THIS);
}
//---------------------------------------------------------------

elapsedMillis since_checked_queues = 0;
Transaction board;

void loop()
{
  if (since_checked_queues > PERIOD_200ms)
  {
    since_checked_queues = 0;

    // changed?
    if (transactionQueue->hasValue() &&
        board.moving != transactionQueue->payload.moving)
    {
      // moving
      if (transactionQueue->payload.moving)
      {
#ifdef NINTENDOCLASSIC_TASK
        nintendoClassTask.enabled = false;
#endif
        remoteTask.enabled = false;
      }
      // stopped
      else
      {
#ifdef NINTENDOCLASSIC_TASK
        nintendoClassTask.enabled = true;
#endif
        remoteTask.enabled = true;
      }

      board.moving = transactionQueue->payload.moving;
    }
  }

  vTaskDelay(TICKS_10ms);
}

//------------------------------------------------------------------

void createQueues()
{
  xBatteryInfo = xQueueCreate(1, sizeof(BatteryInfo *));
  xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(Transaction *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xSimplMessageQueueHandle = xQueueCreate(1, sizeof(SimplMessageObj *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));
}

void createLocalQueueManagers()
{
  transactionQueue = createQueueManager<Transaction>("(main) transactionQueue");
}

void configureTasks()
{
  boardCommsTask.doWorkInterval = PERIOD_50ms;
  boardCommsTask.priority = TASK_PRIORITY_4;
  boardCommsTask.printRadioDetails = PRINT_NRF24L01_DETAILS;
  // boardCommsTask.printBoardPacketAvailable = true;
  boardCommsTask.printFirstBoardPacketAvailable = true;
  // boardCommsTask.printSentPacketToBoard = true;
  // boardCommsTask.printRxQueuePacket = true;
  // boardCommsTask.printTxQueuePacket = true;

#ifdef DIGITALPRIMARYBUTTON_TASK
  digitalPrimaryButtonTask.doWorkInterval = PERIOD_100ms;
  digitalPrimaryButtonTask.priority = TASK_PRIORITY_1;
  // digitalPrimaryButtonTask.printSendToQueue = true;
#endif

  displayTask.doWorkInterval = PERIOD_50ms;
  displayTask.priority = TASK_PRIORITY_2;
  displayTask.p_printState = PRINT_DISP_STATE;
  displayTask.p_printTrigger = PRINT_DISP_STATE_EVENT;

#ifdef HAPTIC_TASK
  hapticTask.priority = TASK_PRIORITY_0;
  hapticTask.doWorkInterval = PERIOD_50ms;
  hapticTask.printDebug = true;
#endif

#ifdef NINTENDOCLASSIC_TASK
  nintendoClassTask.doWorkInterval = PERIOD_50ms;
  nintendoClassTask.priority = TASK_PRIORITY_2;
#endif

#ifdef QWIICBUTTON_TASK
  qwiicButtonTask.doWorkInterval = PERIOD_100ms;
  qwiicButtonTask.priority = TASK_PRIORITY_1;
  qwiicButtonTask.printSendToQueue = true;
#endif

  remoteTask.doWorkInterval = SECONDS * 5;
  remoteTask.priority = TASK_PRIORITY_0;
  remoteTask.printSendToQueue = true;

  statsTask.doWorkInterval = PERIOD_50ms;
  statsTask.priority = TASK_PRIORITY_1;
  // statsTask.printQueueRx = true;
  statsTask.printOnlyFailedPackets = true;

  throttleTask.doWorkInterval = PERIOD_200ms;
  throttleTask.priority = TASK_PRIORITY_3;
  throttleTask.printWarnings = true;
  throttleTask.printThrottle = PRINT_THROTTLE;
  // throttleTask.thumbwheel.setSweepAngle(30.0);
  // throttleTask.thumbwheel.setDeadzone(5.0);
}

void startTasks()
{
  boardCommsTask.start(nsBoardComms::task1);
  displayTask.start(Display::task1);
#ifdef HAPTIC_TASK
  hapticTask.start(nsHapticTask::task1);
#endif
#ifdef NINTENDOCLASSIC_TASK
  nintendoClassTask.start(nsNintendoClassicTask::task1);
#endif
#ifdef QWIICBUTTON_TASK
  qwiicButtonTask.start(nsQwiicButtonTask::task1);
#endif
#ifdef DIGITALPRIMARYBUTTON_TASK
  digitalPrimaryButtonTask.start(nsDigitalPrimaryButtonTask::task1);
#endif
  remoteTask.start(nsRemoteTask::task1);
  statsTask.start(nsStatsTask::task1);
  throttleTask.start(nsThrottleTask::task1);
}

void initialiseTasks()
{
  boardCommsTask.initialiseTask(PRINT_THIS);
  displayTask.initialiseTask(PRINT_THIS);
#ifdef HAPTIC_TASK
  hapticTask.initialiseTask(PRINT_THIS);
#endif
#ifdef NINTENDOCLASSIC_TASK
  nintendoClassTask.initialiseTask(PRINT_THIS);
#endif
#ifdef QWIICBUTTON_TASK
  qwiicButtonTask.initialiseTask(PRINT_THIS);
#endif
#ifdef DIGITALPRIMARYBUTTON_TASK
  digitalPrimaryButtonTask.initialiseTask(PRINT_THIS);
#endif
  remoteTask.initialiseTask(PRINT_THIS);
  statsTask.initialiseTask(PRINT_THIS);
  throttleTask.initialiseTask(PRINT_THIS);
}

void waitForTasks()
{
  while (
      boardCommsTask.ready == false ||
      displayTask.ready == false ||
#ifdef HAPTIC_TASK
      hapticTask.ready == false ||
#endif

#ifdef NINTENDOCLASSIC_TASK
      nintendoClassTask.ready == false ||
#endif
#ifdef QWIICBUTTON_TASK
      qwiicButtonTask.ready == false ||
#endif
#ifdef DIGITALPRIMARYBUTTON_TASK
      digitalPrimaryButtonTask.ready == false ||
#endif
      remoteTask.ready == false ||
      statsTask.ready == false ||
      throttleTask.ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);
  Serial.printf("-- all tasks ready! --\n");
}

void enableTasks(bool print)
{
  boardCommsTask.enable(print);
  displayTask.enable(print);
#ifdef HAPTIC_TASK
  hapticTask.enable(print);
#endif
#ifdef NINTENDOCLASSIC_TASK
  nintendoClassTask.enable(print);
#endif
#ifdef QWIICBUTTON_TASK
  qwiicButtonTask.enable(print);
#endif
#ifdef DIGITALPRIMARYBUTTON_TASK
  digitalPrimaryButtonTask.enable(print);
#endif
  remoteTask.enable(print);
  statsTask.enable(print);
  throttleTask.enable(print);
}

#endif // UNIT_TEST