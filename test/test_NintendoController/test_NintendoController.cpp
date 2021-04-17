#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_MUTEX_TAKE_FAIL 1
static int counter = 0;

// RTOS ENTITES-------------------

QueueHandle_t xFirstQueueHandle;
QueueHandle_t xOtherTestQueueHandle;

QueueHandle_t xBoardPacketQueue;
QueueHandle_t xNintendoControllerQueue;
QueueHandle_t xPacketStateQueueHandle;
QueueHandle_t xPrimaryButtonQueueHandle;
QueueHandle_t xSendToBoardQueueHandle;
QueueHandle_t xThrottleQueueHandle;

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

#include <types.h>
#include <rtosManager.h>
#include <QueueManager1.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>
#include <testUtils.h>
#include <Wire.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

#include <types/QueueBase.h>
#include <types/PacketState.h>
#include <types/SendToBoardNotf.h>
#include <types/NintendoButtonEvent.h>
#include <types/PrimaryButton.h>
#include <types/Throttle.h>

#define RADIO_OBJECTS

// TASKS ------------------------

#include <tasks/core0/QwiicTaskBase.h>
#include <tasks/core0/BaseTaskTest1.h>
#include <tasks/core0/TaskScheduler.h>
#include <tasks/core0/CommsTask.h>
#include <tasks/core0/NintendoClassicTaskBase.h>

//----------------------------------

void printTestTitle(const char *name)
{
  Serial.printf("-------------------------------------------\n");
  Serial.printf("  TEST: %s\n", name);
  Serial.printf("-------------------------------------------\n");
}

void printTestInstructions(const char *instructions)
{
  Serial.printf("*** INSTR: %s\n", instructions);
}

void setUp()
{
  DEBUG("----------------------------");
  Serial.printf("    %s \n", __FILE__);
  DEBUG("----------------------------");

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));
}

void tearDown()
{
}

//-----------------------------------------------
void usesTaskSchedulerAndNintendoController_withTaskBaseAnRealController_sendsPacketsAndRespondsOK()
{
  Wire.begin();
  // start tasks
  TaskScheduler::start();
  TaskScheduler::sendInterval = PERIOD_200ms;
  TaskScheduler::printSendToSchedule = false;

  NintendoClassicTaskBase::start();
  NintendoClassicTaskBase::printReplyToSchedule = false;
  NintendoClassicTaskBase::printPeekSchedule = false;

  // configure queues
  Queue1::Manager<SendToBoardNotf> *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  Queue1::Manager<NintendoButtonEvent> *nintendoButtonEventQueue = Queue1::Manager<NintendoButtonEvent>::create("(test)nintendoButtonEventQueue");

  // mocks
  // - NONE

  // wait
  while (TaskScheduler::thisTask->ready == false ||
         NintendoClassicTaskBase::thisTask->ready == false ||
         false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  vTaskDelay(PERIOD_100ms);

  TaskScheduler::thisTask->enable(PRINT_THIS);
  NintendoClassicTaskBase::thisTask->enable(PRINT_THIS);

  counter = 0;

  const int NUM_LOOPS = 5;
  elapsedMillis since_started_testing = 0;

  while (since_started_testing < 8 * SECONDS)
  {
    // confirm schedule packet on queue
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find schedule packet on the schedule queue");

    // // check for resmary Button pressed: %s\n", pressed);
    response = waitForNew(nintendoButtonEventQueue, PERIOD_20ms);
    TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.correlationId,
                              nintendoButtonEventQueue->payload.correlationId,
                              "nintendoButtonEventQueue correlationId did not match");
    if (nintendoButtonEventQueue->payload.changed &&
        nintendoButtonEventQueue->payload.state == NintendoController::BUTTON_PRESSED)
      Serial.printf("Nintendo button pressed: %s (took %lu to respond)\n",
                    NintendoClassicTaskBase::getButtonName(nintendoButtonEventQueue->payload.button),
                    nintendoButtonEventQueue->payload.getSinceSent());
    counter++;

    vTaskDelay(10);
  }

  TaskScheduler::thisTask->deleteTask(PRINT_THIS);
  NintendoClassicTaskBase::thisTask->deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}
//-----------------------------------------------

//===================================================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(usesTaskSchedulerAndNintendoController_withTaskBaseAnRealController_sendsPacketsAndRespondsOK);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
