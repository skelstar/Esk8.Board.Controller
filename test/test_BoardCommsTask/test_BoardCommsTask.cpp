#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_MUTEX_TAKE_FAIL 1
// TODO pass in to ThrottleBase

// RTOS ENTITES-------------------

#include <tasks/queues/queues.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/queues/types/root.h>

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

#include <types.h>
#include <rtosManager.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>
// #include <Wire.h>

// Mocks
#include <MockQwiicButton.h>
#include <MockNintendoController.h>
#include <MockMagThrottle.h>

#define RADIO_OBJECTS
#include <MockGenericClient.h>
RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);
NRF24L01Lib nrf24;

// TASKS ------------------------

#include <tasks/root.h>
#include <tasks/queues/Managers.h>
#include <testUtils.h>

//----------------------------------

static int counter = 0;

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

  xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));

  // configure queues
  displayEventQueue = Queue1::Manager<DisplayEvent>::create("(test)displayEventQueue");
  primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(test)primaryButtonQueue");
  packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");
  nintendoQueue = Queue1::Manager<NintendoButtonEvent>::create("(test)nintendoQueue");
  throttleQueue = Queue1::Manager<ThrottleState>::create("(test)throttleQueue");

  QwiicTaskBase::start(/*work*/ PERIOD_100ms);
  BoardCommsTask::start(/*work*/ PERIOD_100ms, /*send*/ PERIOD_200ms);
  NintendoClassicTaskBase::start(/*work*/ PERIOD_50ms);
  DisplayTaskBase::start(/*work*/ PERIOD_50ms);
  ThrottleTaskBase::start(/*work*/ PERIOD_200ms);
}

void tearDown()
{
  QwiicTaskBase::thisTask->deleteTask(PRINT_THIS);
  BoardCommsTask::thisTask->deleteTask(PRINT_THIS);
  NintendoClassicTaskBase::thisTask->deleteTask(PRINT_THIS);
  DisplayTaskBase::thisTask->deleteTask(PRINT_THIS);
  ThrottleTaskBase::thisTask->deleteTask(PRINT_THIS);
}

//===================================================================

VescData mockMovingResponse(ControllerData out)
{
  DEBUG("mockMovingResponse called");
  VescData mockresp;
  mockresp.id = out.id;
  mockresp.version = VERSION_BOARD_COMPAT;
  mockresp.moving = false;
  return mockresp;
}

void BoardCommsTask_withMockGenericClient_repliesWithCorrectData()
{
  // tasks are running

  // Mocks
  BoardCommsTask::boardClient.mockResponseCallback(mockMovingResponse);
  BoardCommsTask::printSentPacketToBoard = true;

  Test::waitForTasksReady();
  Test::enableAllTasks();

  vTaskDelay(TICKS_500ms);

  TEST_ASSERT_TRUE(packetStateQueue->hasValue());
  TEST_ASSERT_EQUAL(VERSION_BOARD_COMPAT, packetStateQueue->payload.version);
}

//===================================================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(BoardCommsTask_withMockGenericClient_repliesWithCorrectData);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
