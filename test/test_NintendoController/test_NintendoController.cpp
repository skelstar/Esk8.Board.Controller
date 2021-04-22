#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_MUTEX_TAKE_FAIL 1

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

// Mocks ----
// #include <MockNintendoController.h>
// #include <MockQwiicButton.h>
// #include <GenericClient.h>

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

void setUp()
{
  Test::setupAllTheTasks(__FILE__);
}

void tearDown()
{
  Test::tearDownAllTheTasks();
}

//-----------------------------------------------
void usesTaskSchedulerAndNintendoController_withTaskBaseAnRealController_sendsPacketsAndRespondsOK()
{
  Wire.begin();

  // start tasks
  ThrottleTaskBase::printWarnings = false;
  MagneticThrottle::printThrottle = false;
  BoardCommsTask::printWarnings = false;

  // mocks
  // - NONE

  // wait
  Test::waitForTasksReady();

  Test::enableAllTasks();

  vTaskDelay(PERIOD_100ms);

  counter = 0;

  elapsedMillis since_started_testing = 0;

  bool read_start_button = false;
  Test::printTestInstructions("Press the START button in the next 8 seconds");

  while (since_started_testing < 8 * SECONDS && read_start_button == false)
  {
    if (nintendoQueue->hasValue())
    {
      if (nintendoQueue->payload.button == (uint8_t)NintendoController::BUTTON_START)
        read_start_button = true;
    }

    vTaskDelay(10);
  }

  TEST_ASSERT_TRUE(read_start_button);
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
