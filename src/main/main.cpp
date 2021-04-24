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
#include <rtosManager.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>
#include <Wire.h>
#include <MagThrottle.h>

#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <rom/rtc.h> // for reset reason
#include <shared-utils.h>
#include <types.h>
#include <constants.h>

// TASKS ------------------------

#include <tasks/root.h>
#include <tasks/queues/Managers.h>

//------------------------------------------------------------------

#ifndef SEND_TO_BOARD_INTERVAL
#define SEND_TO_BOARD_INTERVAL 200
#endif
//------------------------------------------------------------------

#include <utils.h>

void createQueues();
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

  createQueues();

  configureTasks();

  startTasks();

  waitForTasks();

  enableTasks();
}
//---------------------------------------------------------------

void loop()
{

  vTaskDelay(TICKS_10ms);
}

//------------------------------------------------------------------

void createQueues()
{
  xBatteryInfo = xQueueCreate(1, sizeof(BatteryInfo *));
  xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));
}

void configureTasks()
{
  throttleTask.printWarnings = true;
  throttleTask.printThrottle = true;

  boardCommsTask.SEND_TO_BOARD_INTERVAL_LOCAL = SEND_TO_BOARD_INTERVAL;
  boardCommsTask.printRadioDetails = PRINT_NRF24L01_DETAILS;

  displayTask.p_printState = PRINT_DISP_STATE;
  displayTask.p_printTrigger = PRINT_DISP_STATE_EVENT;
}

void startTasks()
{
  boardCommsTask.start(TASK_PRIORITY_4, /*work*/ PERIOD_100ms, BoardComms::task1);
  displayTask.start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms, Display::task1);
  nintendoClassTask.start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms, nsNintendoClassicTask::task1);
  QwiicTaskBase::start(TASK_PRIORITY_2, /*work*/ PERIOD_100ms);
  throttleTask.start(TASK_PRIORITY_4, /*work*/ PERIOD_200ms, nsThrottleTask::task1);

  remoteTask.start(TASK_PRIORITY_0, 5 * SECONDS, Remote::task1);
  remoteTask.printSendToQueue = true;
}

void waitForTasks()
{
  while (
      boardCommsTask.ready == false ||
      displayTask.ready == false ||
      nintendoClassTask.ready == false ||
      QwiicTaskBase::thisTask->ready == false ||
      remoteTask.ready == false ||
      throttleTask.ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);
}

void enableTasks(bool print)
{
  boardCommsTask.enable(print);
  displayTask.enable(print);
  nintendoClassTask.enable(print);
  QwiicTaskBase::thisTask->enable(print);
  remoteTask.enable(print);
  throttleTask.enable(print);
}

#endif // UNIT_TEST