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
// #include <MockQwiicButton.h>
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
#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

static int counter = 0;

void setUp()
{
  Test::setupAllTheTasks(__FILE__);
}

void tearDown()
{
  Test::tearDownAllTheTasks();
}

//-----------------------------------------------
void usesTaskSchedulerAndQwiicButton_withTaskBasesAnRealButton_sendsPacketsAndRespondsOK()
{
  Wire.begin();
  // start tasks

  NintendoClassicTaskBase::printWarnings = false;
  ThrottleTask::printWarnings = false;

  Test::printTestInstructions("Click the primaryButton to satify the test");

  Test::waitForTasksReady();

  Test::enableAllTasks();

  elapsedMillis since_started_testing = 0;

  bool primaryButtonPressed = false;

  while (since_started_testing < 5 * SECONDS &&
         primaryButtonPressed == false)
  {
    if (primaryButtonQueue->hasValue() && primaryButtonQueue->payload.pressed)
    {
      primaryButtonPressed = true;
    }

    vTaskDelay(TICKS_100ms);
  }

  TEST_ASSERT_TRUE(primaryButtonPressed);
}

//===================================================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(usesTaskSchedulerAndQwiicButton_withTaskBasesAnRealButton_sendsPacketsAndRespondsOK);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
