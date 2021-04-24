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
  ThrottleTaskBase::settings.printWarnings = false;
  ThrottleTaskBase::settings.printThrottle = PRINT_THROTTLE;

  DisplayTaskBase::settings.printState = PRINT_DISP_STATE;
  DisplayTaskBase::settings.printTrigger = PRINT_DISP_STATE_EVENT;
}

void startTasks()
{
  RemoteTask::start(TASK_PRIORITY_0, /*work*/ 10 * SECONDS);
  BoardCommsTask::start(TASK_PRIORITY_4, /*work*/ PERIOD_100ms, /*send*/ PERIOD_200ms);
  DisplayTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms);
  NintendoClassicTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms);
  QwiicTaskBase::start(TASK_PRIORITY_2, /*work*/ PERIOD_100ms);
  ThrottleTaskBase::start(TASK_PRIORITY_4, /*work*/ PERIOD_200ms);

  remoteTask.start(TASK_PRIORITY_0, 5 * SECONDS, Remote::task1);
  remoteTask.printSendToQueue = true;
}

void waitForTasks()
{
  while (
      BoardCommsTask::thisTask->ready == false ||
      DisplayTaskBase::thisTask->ready == false ||
      NintendoClassicTaskBase::thisTask->ready == false ||
      QwiicTaskBase::thisTask->ready == false ||
      RemoteTask::thisTask->ready == false ||
      ThrottleTaskBase::thisTask->ready == false ||
      remoteTask.ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);
}

void enableTasks(bool print)
{
  BoardCommsTask::thisTask->enable(print);
  DisplayTaskBase::thisTask->enable(print);
  NintendoClassicTaskBase::thisTask->enable(print);
  QwiicTaskBase::thisTask->enable(print);
  RemoteTask::thisTask->enable(print);
  ThrottleTaskBase::thisTask->enable(print);
  remoteTask.enable(print);
}

#endif // UNIT_TEST