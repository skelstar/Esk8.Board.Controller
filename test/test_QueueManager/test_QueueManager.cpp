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
  Test::setupAllTheTasks(__FILE__);
}

void tearDown()
{
  Test::tearDownAllTheTasks();
}

void hasValue_responds_with_correct_value()
{
  ThrottleTask::printWarnings = false;

  BoardCommsTask::boardClient.mockResponseCallback(Test::mockBoardStoppedResponse);

  Test::printTestInstructions("Test calls and responds in same task");

  Test::waitForTasksReady();

  Test::enableAllTasks();

  vTaskDelay(PERIOD_500ms);

  while (counter < 5)
  {
    TEST_ASSERT_TRUE(packetStateQueue->hasValue());
    counter++;

    vTaskDelay(PERIOD_500ms);
  }
  TEST_ASSERT_TRUE(counter >= 5);
}

//===================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(hasValue_responds_with_correct_value);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
