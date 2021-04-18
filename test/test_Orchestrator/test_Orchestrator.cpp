#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_MUTEX_TAKE_FAIL 1

// RTOS ENTITES-------------------

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

// MyMutex mutex_I2C;
// MyMutex mutex_SPI;

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

// TASKS ------------------------
// bases
#include <tasks/core0/CommsTask.h>
#include <tasks/core0/NintendoClassicTaskBase.h>
#include <tasks/core0/OrchestratorTask.h>
#include <tasks/core0/QwiicTaskBase.h>
// #include <tasks/core0/TaskScheduler.h>

//----------------------------------

static int counter = 0;
elapsedMillis since_started_testing = 0;

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

  counter = 0;
}

void tearDown()
{
}

//-----------------------------------------------
void OrchestratorTask_sendPacketsRegularly()
{
  // start tasks
  OrchestratorTask::start();
  OrchestratorTask::sendInterval = PERIOD_200ms;
  OrchestratorTask::printSendToQueue = true;

  // configure queues
  auto *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");

  // mocks

  // wait
  while (OrchestratorTask::thisTask->ready == false ||
         false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  vTaskDelay(PERIOD_100ms);

  OrchestratorTask::thisTask->enable(PRINT_THIS);

  counter = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // confirm schedule packet on queue
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S, nullptr, PRINT_TIMEOUT);
    TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find schedule packet on the schedule queue");

    counter++;

    vTaskDelay(10);
  }

  OrchestratorTask::thisTask->deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

//-----------------------------------------------
void OrchestratorTask_usesBroadcastToGetResponses_getsResponseswhenRequested()
{
  // start tasks
  QwiicTaskBase::start();
  // QwiicTaskBase::printPeekSchedule = true;
  QwiicTaskBase::printReplyToSchedule = true;
  QwiicTaskBase::thisTask->doWorkInterval = PERIOD_50ms;

  // configure queues
  auto *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  auto *primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(test)primaryButtonQueue");

  // mocks
  QwiicTaskBase::qwiicButton.setMockIsPressedCallback([] {
    return false;
  });

  // wait
  while (QwiicTaskBase::thisTask->ready == false ||
         false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  // QwiicTaskBase::thisTask->enable(PRINT_THIS);

  vTaskDelay(PERIOD_500ms);

  counter = 0;

  // clear the queue
  scheduleQueue->read();

  SendToBoardNotf notification;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    notification.command = counter % 2 == 0
                               ? QueueBase::Command::RESPOND
                               : QueueBase::Command::NONE;

    DEBUGMVAL("Before sending: ", millis(), notification.correlationId, notification.command);
    vTaskDelay(PERIOD_20ms);
    scheduleQueue->send_r(&notification, QueueBase::printSend);

    vTaskDelay(TICKS_10ms);

    QwiicTaskBase::thisTask->enable(PRINT_THIS);

    // confirm schedule packet on queue
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S);
    TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response,
                              "Didn't find schedule packet on the schedule queue");
    TEST_ASSERT_EQUAL_MESSAGE(notification.command, scheduleQueue->payload.command,
                              "Didn't get the correct command from the schedule queue");
    Serial.printf("[PASS @%lums] Found packet on Schedule queue\n", millis());

    vTaskDelay(TICKS_50ms);
    // make sure no more packets on Schedule queue
    response = waitForNew(scheduleQueue, PERIOD_50ms);
    TEST_ASSERT_EQUAL_MESSAGE(Response::TIMEOUT, response,
                              "Did find schedule packet on the schedule queue");
    Serial.printf("[PASS @%lums] Didn't find another packet on Schedule queue\n", millis());

    // confirm broadcast response
    response = waitForNew(primaryButtonQueue, PERIOD_50ms);
    if (notification.command == QueueBase::Command::RESPOND)
    {
      TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response, "Device was supposed to respond but timed out");
      Serial.printf("[PASS @%lums] Responded on PrimaryButton queue\n", millis());
    }
    else
    {
      TEST_ASSERT_EQUAL_MESSAGE(Response::TIMEOUT, response, "Device was supposed to time out but responded");
      Serial.printf("[PASS @%lums] Timedout on PrimaryButton queue\n", millis());
    }

    counter++;

    Serial.printf("%d----------------------------\n", counter);

    vTaskDelay(10);
  }

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);

  QwiicTaskBase::thisTask->deleteTask(PRINT_THIS);
}

//-----------------------------------------------
void OrchestratorTask_usesBroadcastToGetResponses_getsResponsesFromOtherTaskswhenRequested()
{
  // start tasks
  QwiicTaskBase::start();
  // QwiicTaskBase::printPeekSchedule = false;
  // QwiicTaskBase::printReplyToSchedule = true;
  QwiicTaskBase::thisTask->doWorkInterval = PERIOD_50ms;

  CommsTask::start();
  CommsTask::thisTask->doWorkInterval = PERIOD_50ms;
  CommsTask::printSendNewPacket = true;
  CommsTask::printPeekSchedule = true;
  CommsTask::SEND_TO_BOARD_INTERVAL_LOCAL = PERIOD_500ms;

  // configure queues
  auto *scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  auto *primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(test)primaryButtonQueue");
  auto *packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");

  // mocks ---
  QwiicTaskBase::qwiicButton.setMockIsPressedCallback([] {
    return false;
  });

  CommsTask::boardClient.mockResponseCallback([](ControllerData out) {
    // DEBUG("mockMovingResponse called");
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = false;
    return mockresp;
  });

  // wait ---
  while (QwiicTaskBase::thisTask->ready == false ||
         CommsTask::thisTask->ready == false ||
         false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  QwiicTaskBase::thisTask->enable(PRINT_THIS);
  CommsTask::thisTask->enable(PRINT_THIS);

  vTaskDelay(PERIOD_500ms);

  counter = 0;

  // clear the queue
  scheduleQueue->read();

  SendToBoardNotf notification;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    notification.command = counter % 2 == 0
                               ? QueueBase::Command::RESPOND
                               : QueueBase::Command::NONE;
    vTaskDelay(PERIOD_20ms);
    scheduleQueue->send_r(&notification, QueueBase::printSend);

    // confirm schedule packet on queue
    uint8_t response = waitForNew(scheduleQueue, PERIOD_1S);
    TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response,
                              "Didn't find schedule packet on the schedule queue");
    TEST_ASSERT_EQUAL_MESSAGE(notification.command, scheduleQueue->payload.command,
                              "Didn't get the correct command from the schedule queue");

    // confirm broadcast response
    response = waitForNew(primaryButtonQueue, PERIOD_50ms);
    if (notification.command == QueueBase::Command::RESPOND)
      TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response, "QwiicTask was supposed to respond but timed out");
    else
      TEST_ASSERT_EQUAL_MESSAGE(Response::TIMEOUT, response, "QwiicTask was supposed to time out but responded");

    response = waitForNew(packetStateQueue, PERIOD_50ms);
    if (notification.command == QueueBase::Command::RESPOND)
      TEST_ASSERT_EQUAL_MESSAGE(Response::OK, response, "CommsTask was supposed to respond but timed out");
    else
      TEST_ASSERT_EQUAL_MESSAGE(Response::TIMEOUT, response, "CommsTask was supposed to time out but responded");

    counter++;

    vTaskDelay(TICKS_1s);
  }

  vTaskDelay(PERIOD_1S);

  OrchestratorTask::thisTask->deleteTask();
  QwiicTaskBase::thisTask->deleteTask();
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

  // RUN_TEST(OrchestratorTask_sendPacketsRegularly);
  RUN_TEST(OrchestratorTask_usesBroadcastToGetResponses_getsResponseswhenRequested);
  // RUN_TEST(OrchestratorTask_usesBroadcastToGetResponses_getsResponsesFromOtherTaskswhenRequested);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
