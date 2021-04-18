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

// #include <MockedQwiicButton.h>
// #include <MockNintendoController.h>

// #include <displayState.h>

// TASKS ------------------------

// #include <tasks/core0/OrchestratorTask.h>
// #include <tasks/core0/DisplayTask.h>
// #include <tasks/core0/QwiicTaskBase.h>
// #include <tasks/core0/ThrottleTask.h>
// #include <tasks/core0/BoardCommsTask.h>
// #include <tasks/core0/NintendoClassicTask.h>
// #include <tasks/core0/remoteTask.h>

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

//===================================================================

void usesTaskScheduler_repliesWithPacketsOK()
{
  // start tasks
  TaskScheduler::start();
  TaskScheduler::sendInterval = PERIOD_500ms;
  TaskScheduler::printSendToSchedule = true;

  CommsTask::start();
  CommsTask::printReplyToSchedule = true;
  CommsTask::printPeekSchedule = true;

  // configure queues
  auto *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  auto *packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");

  // wait
  while (TaskScheduler::thisTask->ready == false ||
         CommsTask::thisTask->ready == false ||
         false)
    vTaskDelay(10);

  DEBUG("Tasks ready");

  TaskScheduler::thisTask->enable();
  CommsTask::thisTask->enable();

  counter = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // check for Notf
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find notification on the queue");

    // check for packet response
    response = waitForNew(packetStateQueue, PERIOD_50ms, nullptr, PRINT_TIMEOUT);
    PacketState payload = packetStateQueue->payload;
    TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response, "Didn't find the PacketState on the queue");
    TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.correlationId, payload.correlationId, "Didn't find the correct correlationId");

    counter++;

    vTaskDelay(10);
  }

  TaskScheduler::thisTask->deleteTask();
  CommsTask::thisTask->deleteTask();

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

//-------------------------------------------------------

VescData mockMovingResponse(ControllerData out)
{
  // DEBUG("mockMovingResponse called");
  VescData mockresp;
  mockresp.id = out.id;
  mockresp.version = VERSION_BOARD_COMPAT;
  mockresp.moving = false;
  return mockresp;
}

void usesTaskScheduler_withMockGenericClient_repliesWithCorrectData()
{
  // start tasks
  TaskScheduler::start();
  TaskScheduler::sendInterval = PERIOD_500ms;
  TaskScheduler::printSendToSchedule = true;

  CommsTask::start();
  CommsTask::printReplyToSchedule = true;
  CommsTask::printPeekSchedule = false;

  // configure queues
  auto *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  auto *packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");

  // mocks
  CommsTask::boardClient.mockResponseCallback(mockMovingResponse);

  // wait
  while (TaskScheduler::thisTask->ready == false ||
         CommsTask::thisTask->ready == false ||
         false)
    vTaskDelay(10);

  DEBUG("Tasks ready");

  TaskScheduler::thisTask->enable();
  CommsTask::thisTask->enable();

  counter = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // check for Notf
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find notification on the queue");

    // check for packet response
    response = waitForNew(packetStateQueue, PERIOD_50ms, nullptr, PRINT_TIMEOUT);
    PacketState payload = packetStateQueue->payload;
    TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response, "Didn't find the PacketState on the queue");
    TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.correlationId, payload.correlationId, "Didn't find the correct correlationId");

    counter++;

    vTaskDelay(10);
  }

  TaskScheduler::thisTask->deleteTask();
  CommsTask::thisTask->deleteTask();

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

//===================================================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  // RUN_TEST(usesTaskScheduler_repliesWithPacketsOK);
  RUN_TEST(usesTaskScheduler_withMockGenericClient_repliesWithCorrectData);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
