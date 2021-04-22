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
// #include <GenericClient.h>

#define RADIO_OBJECTS
#include <MockGenericClient.h>
RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);
NRF24L01Lib nrf24;

// TASKS ------------------------

#include <tasks/root.h>
#include <tasks/queues/Managers.h>
#include <tasks/queues/QueueFactory.h>
#include <testUtils.h>

//----------------------------------

static int counter = 0;

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

  counter = 0;
}

void tearDown()
{
  QwiicTaskBase::thisTask->deleteTask(PRINT_THIS);
  BoardCommsTask::thisTask->deleteTask(PRINT_THIS);
  NintendoClassicTaskBase::thisTask->deleteTask(PRINT_THIS);
  DisplayTaskBase::thisTask->deleteTask(PRINT_THIS);
  ThrottleTaskBase::thisTask->deleteTask(PRINT_THIS);
}

//-----------------------------------------------
void BoardIsStopped_whenMoving_showsTheMovingScreen()
{
  Wire.begin();

  DisplayTaskBase::settings.printState = true;
  DisplayTaskBase::settings.printTrigger = true;

  /* #region  Mocks */

  BoardCommsTask::boardClient.mockResponseCallback([](ControllerData out) {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = counter >= 2;
    // Serial.printf("[%lu] mockMovingResponse called, moving: %d\n", millis(), mockresp.moving);
    return mockresp;
  });

  /* #endregion */

  while (
      QwiicTaskBase::thisTask->ready == false ||
      BoardCommsTask::thisTask->ready == false ||
      NintendoClassicTaskBase::thisTask->ready == false ||
      DisplayTaskBase::thisTask->ready == false ||
      false)
  {
    vTaskDelay(10);
  }
  DEBUG("Tasks ready");

  /* #region  enable tasks */

  QwiicTaskBase::thisTask->enable(PRINT_THIS);
  BoardCommsTask::thisTask->enable(PRINT_THIS);
  NintendoClassicTaskBase::thisTask->enable(PRINT_THIS);
  DisplayTaskBase::thisTask->enable(PRINT_THIS);

  /* #endregion */

  vTaskDelay(PERIOD_500ms);

  counter = 0;

  const int NUM_LOOPS = 5;

  // tests
  while (counter < NUM_LOOPS)
  {
    if (counter == 1)
    {
      TEST_ASSERT_TRUE_MESSAGE(Display::fsm_mgr.currentStateIs(Display::ST_STOPPED_SCREEN), "Display is not showing STOPPED screen");
    }
    else if (counter > 2)
    {
      TEST_ASSERT_TRUE_MESSAGE(Display::fsm_mgr.currentStateIs(Display::ST_MOVING_SCREEN), "Display is not showing MOVING screen");
    }

    Serial.printf("Counter: %d\n", counter);

    counter++;

    vTaskDelay(TICKS_1s);
  }

  vTaskDelay(PERIOD_1s);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}
//-----------------------------------------------

void NintendoStartButton_whenPressed_opensPushToStartScreen()
{
  Wire.begin();

  DisplayTaskBase::settings.printState = true;
  DisplayTaskBase::settings.printTrigger = true;

  /* #region  Mocks */

  QwiicTaskBase::qwiicButton.setMockIsPressedCallback([] {
    return false;
  });

  NintendoClassicTaskBase::classic.setMockGetButtonEventCallback([] {
    switch (counter)
    {
    case 2:
      return (uint8_t)NintendoController::BUTTON_START;
    }
    return (uint8_t)NintendoController::BUTTON_NONE;
  });

  BoardCommsTask::boardClient.mockResponseCallback([](ControllerData out) {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = false;
    // Serial.printf("[%lu] mockMovingResponse called, moving: %d\n", millis(), mockresp.moving);
    return mockresp;
  });

  /* #endregion */

  Test::waitForTasksReady();

  Test::enableAllTasks();

  vTaskDelay(PERIOD_500ms);

  counter = 0;

  const int NUM_LOOPS = 5;

  // tests
  while (counter < NUM_LOOPS)
  {
    if (counter == 1)
    {
      TEST_ASSERT_TRUE_MESSAGE(Display::fsm_mgr.currentStateIs(Display::ST_STOPPED_SCREEN), "Display is not showing STOPPED screen");
    }
    else if (counter == 2)
    {
      TEST_ASSERT_TRUE_MESSAGE(Display::fsm_mgr.currentStateIs(Display::ST_OPTION_PUSH_TO_START), "Display is not showing OPTION_PUSH_TO_START screen");
    }
    else if (counter == 4)
    {
      TEST_ASSERT_TRUE_MESSAGE(Display::fsm_mgr.currentStateIs(Display::ST_STOPPED_SCREEN), "Display is not showing MOVING screen");
    }

    Serial.printf("Counter: %d\n", counter);

    counter++;

    vTaskDelay(TICKS_1s);
  }

  vTaskDelay(PERIOD_1s);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

//===================================================================

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(BoardIsStopped_whenMoving_showsTheMovingScreen);
  // RUN_TEST(NintendoStartButton_whenPressed_opensPushToStartScreen);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
