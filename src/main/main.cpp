#include <Arduino.h>
#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <tasks/queues/queues.h>
#include <tasks/queues/types/root.h>

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

#include <types.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <Wire.h>

#include <constants.h>

#if REMOTE_USED == NINTENDO_REMOTE
#include <SparkFun_Qwiic_Button.h>
#include <MagThumbwheel.h>
#elif REMOTE_USED == PURPLE_REMOTE
#include <AnalogI2CTrigger.h>
// #include <AnalogThumbwheel.h>
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

int tasksCount = 0;

TaskBase *tasks[NUM_TASKS];

// LOCAL QUEUE MANAGERS -----------------------

Queue1::Manager<Transaction> *transactionQueue = nullptr;

//------------------------------------------------------------------

#include <utils.h>

// prototypes
void populateTaskList();
void createQueues();
void createLocalQueueManagers();
void startTasks();
void initialiseTasks();
void configureTasks();
void waitForTasks();
void enableTasks(bool print = false);
void handleTransaction(Transaction &transaction, bool movingChange);

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

  populateTaskList();

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
  if (since_checked_queues > PERIOD_100ms)
  {
    since_checked_queues = 0;

    // changed?
    if (transactionQueue->hasValue())
      handleTransaction(transactionQueue->payload, /*moving change*/ board.moving != transactionQueue->payload.moving);

    board.moving = transactionQueue->payload.moving;
  }

  vTaskDelay(TICKS_10ms);
}

//------------------------------------------------------------------

void addTaskToList(TaskBase *t)
{
  Serial.printf("Adding task: %s\n", t->_name);
  if (tasksCount < NUM_TASKS)
  {
    tasks[tasksCount++] = t;
    assert(tasksCount < NUM_TASKS);
  }
}

void populateTaskList()
{
  addTaskToList(&boardCommsTask);
  addTaskToList(&remoteTask);
  addTaskToList(&throttleTask);

#ifdef DISPLAY_TASK
  addTaskToList(&displayTask);
#endif
#ifdef STATS_TASK
  addTaskToList(&statsTask);
#endif
#ifdef DIGITALPRIMARYBUTTON_TASK
  addTaskToList(&digitalPrimaryButtonTask);
#endif
#ifdef HAPTIC_TASK
  addTaskToList(&hapticTask);
#endif
#ifdef NINTENDOCLASSIC_TASK
  addTaskToList(&nintendoClassTask);
#endif
#ifdef QWIICBUTTON_TASK
  addTaskToList(&qwiicButtonTask);
#endif
}
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
  transactionQueue->printMissedPacket = false;
}

void configureTasks()
{
  DEBUG("Configuring tasks");

  boardCommsTask.doWorkIntervalFast = PERIOD_50ms;
  boardCommsTask.priority = TASK_PRIORITY_4;
  boardCommsTask.printRadioDetails = PRINT_NRF24L01_DETAILS;
  // boardCommsTask.printBoardPacketAvailable = true;
  boardCommsTask.printFirstBoardPacketAvailable = true;
  // boardCommsTask.printSentPacketToBoard = true;
  // boardCommsTask.printRxQueuePacket = true;
  // boardCommsTask.printTxQueuePacket = true;

#ifdef DIGITALPRIMARYBUTTON_TASK
  digitalPrimaryButtonTask.doWorkIntervalFast = PERIOD_100ms;
  digitalPrimaryButtonTask.priority = TASK_PRIORITY_1;
  // digitalPrimaryButtonTask.printSendToQueue = true;
#endif

#ifdef DISPLAY_TASK
  displayTask.doWorkIntervalFast = PERIOD_50ms;
  displayTask.doWorkIntervalSlow = PERIOD_500ms;
  displayTask.priority = TASK_PRIORITY_2;
  displayTask.p_printState = PRINT_DISP_STATE;
  displayTask.p_printTrigger = PRINT_DISP_STATE_EVENT;
#endif

#ifdef HAPTIC_TASK
  hapticTask.priority = TASK_PRIORITY_0;
  hapticTask.doWorkIntervalFast = PERIOD_50ms;
  hapticTask.printDebug = true;
  hapticTask.printFsmTrigger = false;
#endif

#ifdef NINTENDOCLASSIC_TASK
  nintendoClassTask.doWorkIntervalFast = PERIOD_50ms;
  nintendoClassTask.priority = TASK_PRIORITY_2;
#endif

#ifdef QWIICBUTTON_TASK
  qwiicButtonTask.doWorkIntervalFast = PERIOD_100ms;
  qwiicButtonTask.priority = TASK_PRIORITY_1;
  qwiicButtonTask.printSendToQueue = true;
#endif

  remoteTask.doWorkIntervalFast = SECONDS * 5;
  remoteTask.priority = TASK_PRIORITY_0;
  remoteTask.printSendToQueue = true;

#ifdef STATS_TASK
  statsTask.doWorkIntervalFast = PERIOD_50ms;
  statsTask.priority = TASK_PRIORITY_1;
  // statsTask.printQueueRx = true;
  statsTask.printOnlyFailedPackets = true;
#endif

  throttleTask.doWorkIntervalFast = PERIOD_200ms;
  throttleTask.priority = TASK_PRIORITY_3;
  throttleTask.printWarnings = true;
  throttleTask.printThrottle = PRINT_THROTTLE;
  // throttleTask.thumbwheel.setSweepAngle(30.0);
  // throttleTask.thumbwheel.setDeadzone(5.0);
}

void startTasks()
{
  DEBUG("Starting tasks");

  boardCommsTask.start(nsBoardComms::task1);
  remoteTask.start(nsRemoteTask::task1);
  throttleTask.start(nsThrottleTask::task1);

#ifdef DISPLAY_TASK
  displayTask.start(Display::task1);
#endif
#ifdef STATS_TASK
  statsTask.start(nsStatsTask::task1);
#endif
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
}

void initialiseTasks()
{
  DEBUG("Initialising tasks");

  for (int i = 0; i < tasksCount; i++)
    tasks[i]->initialiseTask(PRINT_THIS);
}

void waitForTasks()
{
  bool allReady = false;
  while (!allReady)
  {
    allReady = true;
    for (int i = 0; i < tasksCount; i++)
      allReady = allReady && tasks[i]->ready;
    vTaskDelay(PERIOD_100ms);
    DEBUG("Waiting for tasks\n");
  }
  DEBUG("-- all tasks ready! --");
}

void enableTasks(bool print)
{
  for (int i = 0; i < tasksCount; i++)
    tasks[i]->enable(print);
}

void handleTransaction(Transaction &transaction, bool movingChange)
{
  if (movingChange)
  { // moving
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
  }
}

#endif // UNIT_TEST