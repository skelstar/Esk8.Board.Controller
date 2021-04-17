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

MyMutex mutex_I2C;
MyMutex mutex_SPI;

#include <types/QueueBase.h>
#include <types/PacketState.h>
#include <types/SendToBoardNotf.h>
#include <types/NintendoButtonEvent.h>
#include <types/PrimaryButton.h>
#include <types/Throttle.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <MockGenericClient.h>
// #include <GenericClient.h>

#define RADIO_OBJECTS
// NRF24L01Lib nrf24;

// RF24 radio(NRF_CE, NRF_CS);
// RF24Network network(radio);
GenericClient<ControllerData, VescData> boardClient(01);

#include <MockedQwiicButton.h>
#include <MockNintendoController.h>

#include <displayState.h>

// TASKS ------------------------

#include <tasks/core0/SendToBoardTimerTask.h>
#include <tasks/core0/DisplayTask.h>
#include <tasks/core0/QwiicTaskBase.h>
#include <tasks/core0/ThrottleTask.h>
#include <tasks/core0/BoardCommsTask.h>
#include <tasks/core0/NintendoClassicTask.h>
#include <tasks/core0/remoteTask.h>

#include <tasks/core0/BaseTaskTest1.h>
#include <tasks/core0/TaskScheduler.h>
#include <tasks/core0/CommsTask.h>

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

// Queue1::Manager<SendToBoardNotf> *sendNotfQueue;
// Queue1::Manager<PrimaryButtonState> *primaryButtonQueue;
// Queue1::Manager<ThrottleState> *readThrottleQueue;
// Queue1::Manager<PacketState> *readPacketStateQueue;
// Queue1::Manager<NintendoButtonEvent> *readNintendoQueue;

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

void usesSendNotfTimer_sendsPacketsOK()
{
  // start tasks
  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  SendToBoardTimerTask::setSendInterval(PERIOD_500ms, PRINT_THIS);

  // configure queues
  Queue1::Manager<SendToBoardNotf> *readNoftQueue = Queue1::Manager<SendToBoardNotf>::create("(test)readNotfQueue");

  // wait
  while (SendToBoardTimerTask::mgr.ready == false)
    vTaskDelay(10);

  SendToBoardTimerTask::mgr.enable(PRINT_THIS);

  DEBUG("Tasks ready");

  counter = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // check for Notf
    uint8_t response = waitForNew(readNoftQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find notification on the queue");

    counter++;

    vTaskDelay(10);
  }

  SendToBoardTimerTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void usesTaskScheduler_withTaskBase_sendsPacketsOK()
{
  // start tasks
  TaskScheduler::start();
  TaskScheduler::sendInterval = PERIOD_500ms;
  TaskScheduler::printSendToSchedule = true;

  // configure queues
  Queue1::Manager<SendToBoardNotf> *readNoftQueue = Queue1::Manager<SendToBoardNotf>::create("(test)readNotfQueue");

  // wait
  while (TaskScheduler::thisTask->ready == false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  TaskScheduler::thisTask->enable(PRINT_THIS);

  counter = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // check for Notf
    uint8_t response = waitForNew(readNoftQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find notification on the queue");

    counter++;

    vTaskDelay(10);
  }

  TaskScheduler::thisTask->deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void usesTaskSchedulerAndCommsTask_withTaskBases_sendsPacketsAndRespondsOK()
{
  // start tasks
  TaskScheduler::start();
  TaskScheduler::sendInterval = PERIOD_200ms; // PERIOD_500ms;
  TaskScheduler::printSendToSchedule = false;

  CommsTask::start();
  CommsTask::printReplyToSchedule = true;

  // configure queues
  Queue1::Manager<SendToBoardNotf> *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  Queue1::Manager<PacketState> *packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");

  // wait
  while (TaskScheduler::thisTask->ready == false ||
         CommsTask::thisTask->ready == false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  TaskScheduler::thisTask->enable(PRINT_THIS);
  CommsTask::thisTask->enable(PRINT_THIS);

  counter = 0;

  const int NUM_LOOPS = 20;

  while (counter < NUM_LOOPS)
  {
    // confirm schedule packet on queue
    uint8_t response = waitForNew(scheduleQueue, PERIOD_300ms, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find schedule packet on the queue");

    // check for response from CommsTask
    response = waitForNew(packetStateQueue, PERIOD_50ms, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find packetState on the queue");
    TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.correlationId,
                              packetStateQueue->payload.correlationId,
                              "PacketState correlationId does not match");
    counter++;

    vTaskDelay(10);
  }

  TaskScheduler::thisTask->deleteTask(PRINT_THIS);
  CommsTask::thisTask->deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void usesTaskSchedulerAndCommsTaskAndQwiicButton_withTaskBases_sendsPacketsAndRespondsOK()
{
  // start tasks
  TaskScheduler::start();
  TaskScheduler::sendInterval = PERIOD_500ms;
  TaskScheduler::printSendToSchedule = false;

  CommsTask::start();
  CommsTask::printReplyToSchedule = true;

  QwiicTaskBase::start();
  QwiicTaskBase::printReplyToSchedule = true;

  // configure queues
  Queue1::Manager<SendToBoardNotf> *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  Queue1::Manager<PacketState> *packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");
  Queue1::Manager<PrimaryButtonState> *primaryButtonStateQueue = Queue1::Manager<PrimaryButtonState>::create("(test)primaruButtonStateQueue");

  // wait
  while (TaskScheduler::thisTask->ready == false ||
         CommsTask::thisTask->ready == false ||
         QwiicTaskBase::thisTask->ready == false ||
         false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  vTaskDelay(PERIOD_100ms);

  TaskScheduler::thisTask->enable(PRINT_THIS);
  CommsTask::thisTask->enable(PRINT_THIS);
  QwiicTaskBase::thisTask->enable(PRINT_THIS);

  counter = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // confirm schedule packet on queue
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find schedule packet on the schedule queue");

    // check for response from CommsTask
    response = waitForNew(packetStateQueue, PERIOD_50ms, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find packetState on the queue");
    TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.correlationId,
                              packetStateQueue->payload.correlationId,
                              "PacketState correlationId does not match");

    // check for response from Primary Button (Qwiic)
    response = waitForNew(primaryButtonStateQueue, PERIOD_50ms, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find primaryButton on the queue");
    TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.correlationId,
                              packetStateQueue->payload.correlationId,
                              "PrimaryButtonState correlationId does not match");
    counter++;

    vTaskDelay(10);
  }

  TaskScheduler::thisTask->deleteTask(PRINT_THIS);
  CommsTask::thisTask->deleteTask(PRINT_THIS);
  QwiicTaskBase::thisTask->deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(usesSendNotfTimer_sendsPacketsOK);
  // RUN_TEST(usesTaskScheduler_withTaskBase_sendsPacketsOK);
  // RUN_TEST(usesTaskSchedulerAndCommsTask_withTaskBases_sendsPacketsAndRespondsOK);
  RUN_TEST(usesTaskSchedulerAndCommsTaskAndQwiicButton_withTaskBases_sendsPacketsAndRespondsOK);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
