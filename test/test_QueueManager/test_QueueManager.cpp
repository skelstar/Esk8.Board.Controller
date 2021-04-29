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

#include <RF24Network.h>
#include <NRF24L01Lib.h>

// mocks
#include <MockNintendoController.h>
#include <MockQwiicButton.h>
#include <MockMagThrottle.h>

#define RADIO_OBJECTS 1
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

void setUp()
{
}

void tearDown()
{
}
//===================================================================
void hasValue_responds_with_correct_value()
{
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));

  packetStateQueue = createQueueManager<PacketState>("(test)packetStateQueue");
  throttleQueue = createQueueManager<ThrottleState>("(test)throttleQueue");

  boardCommsTask.start(BoardComms::task1);

  throttleTask.printWarnings = false;

  Serial.printf("started test\n");

  Test::printTestInstructions("Test calls and responds in same task");

  while (
      boardCommsTask.ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);

  while (boardCommsTask.boardClient == nullptr)
    vTaskDelay(10);
  boardCommsTask.boardClient->mockResponseCallback(Test::mockBoardStoppedResponse);

  Test::enableAllTasks();

  vTaskDelay(PERIOD_500ms);

  while (counter < 5)
  {
    TEST_ASSERT_TRUE(packetStateQueue->hasValue());
    counter++;

    vTaskDelay(PERIOD_500ms);
  }
  TEST_ASSERT_TRUE(counter >= 5);

  Test::tearDownAllTheTasks();
}
//=================================================================

class RepeaterTask : public TaskBase
{
public:
  PacketState *packet;

  Queue1::Manager<PacketState> *packetStateQueue = nullptr;

  RepeaterTask() : TaskBase("RepeaterTask", 3000, PERIOD_20ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_3;
  }
  //----------------------------------------------------------
  void initialiseQueues()
  {
    packetStateQueue = createQueueManager<PacketState>("(RepeaterTask)PacketQueue");
  }
  //----------------------------------------------------------
  void initialise()
  {
    packet = new PacketState();
    packetStateQueue->read(); // clear the queue
  }
  //----------------------------------------------------------

  void doWork()
  {
    if (packetStateQueue->hasValue())
    {
      packet = new PacketState(packetStateQueue->payload);
      PacketState::print(*packet, "-->[Task]");

      packet->moving = 1;
      packetStateQueue->send(packet);
      PacketState::print(*packet, "[Task]-->");
    }
  }

  void cleanup()
  {
    delete (packetStateQueue);
  }
};

RepeaterTask repeaterTask;

namespace nsRepeaterTask
{
  void task1(void *parameters)
  {
    repeaterTask.task(parameters);
  }
}

void twoTasks_usingSameQueue_canReadAndRespond()
{
  PacketState *packet = new PacketState();

  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));

  packetStateQueue = createQueueManager<PacketState>("(Test)packetStateQueue");

  repeaterTask.start(nsRepeaterTask::task1);

  Serial.printf("started test\n");

  Test::printTestInstructions("Test sends packet, task sends response on same queue, test reads response");

  while (
      repeaterTask.ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);

  repeaterTask.enable();

  vTaskDelay(PERIOD_500ms);

  elapsedMillis since_sent;

  while (counter < 5)
  {
    DEBUG("------------------------------");
    packet->moving = 0;
    packetStateQueue->send(packet);
    since_sent = 0;
    ulong sentId = packet->event_id;
    PacketState::print(*packet, "[Test]-->");

    while (!packetStateQueue->hasValue() && since_sent < PERIOD_200ms)
    {
      vTaskDelay(TICKS_50ms);
    }
    packet = new PacketState(packetStateQueue->payload);
    PacketState::print(*packet, "-->[Test]");

    TEST_ASSERT_EQUAL(sentId + 1, packet->event_id);
    TEST_ASSERT_TRUE(packet->moving == 1);
    counter++;

    vTaskDelay(PERIOD_200ms);
  }
  TEST_ASSERT_TRUE(counter >= 5);

  Test::tearDownAllTheTasks();
}

//===================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  // RUN_TEST(hasValue_responds_with_correct_value);
  RUN_TEST(twoTasks_usingSameQueue_canReadAndWrite);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
