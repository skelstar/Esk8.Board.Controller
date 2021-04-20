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

#include <types/DisplayEvent.h>
#include <types/NintendoButtonEvent.h>
#include <types/PacketState.h>
#include <types/PrimaryButton.h>
#include <types/QueueBase.h>
#include <types/SendToBoardNotf.h>
#include <types/Throttle.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

// mocks
#include <MockGenericClient.h>
#include <MockNintendoController.h>
#include <MockQwiicButton.h>
// #include <GenericClient.h>

#define RADIO_OBJECTS
// NRF24L01Lib nrf24;

// RF24 radio(NRF_CE, NRF_CS);
// RF24Network network(radio);
GenericClient<ControllerData, VescData> boardClient(01);

// TASKS ------------------------
// bases
#include <tasks/core0/CommsTask.h>
#include <tasks/core0/NintendoClassicTaskBase.h>
#include <tasks/core0/OrchestratorTask.h>
#include <tasks/core0/QwiicTaskBase.h>
#include <tasks/core0/DisplayTaskBase.h>

Queue1::Manager<SendToBoardNotf> *scheduleQueue = nullptr;
Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;
Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
Queue1::Manager<PacketState> *packetStateQueue = nullptr;
Queue1::Manager<NintendoButtonEvent> *nintendoQueue = nullptr;

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
  xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));

  // configure queues
  scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(test)scheduleQueue");
  displayEventQueue = Queue1::Manager<DisplayEvent>::create("(test)displayEventQueue");
  primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(test)primaryButtonQueue");
  packetStateQueue = Queue1::Manager<PacketState>::create("(test)packetStateQueue");
  nintendoQueue = Queue1::Manager<NintendoButtonEvent>::create("(test)nintendoQueue");

  counter = 0;
}

void tearDown()
{
}

void printPASS(const char *message)
{
  Serial.printf("[PASS @%lums] %s\n", millis(), message);
}

//-----------------------------------------------

void DisplayTask_stuff()
{
  Wire.begin();
  // start tasks
  namespace qwiicT_ = QwiicTaskBase;
  qwiicT_::start();
  qwiicT_::thisTask->doWorkInterval = PERIOD_50ms;

  namespace commsT_ = CommsTask;
  commsT_::start();
  commsT_::thisTask->doWorkInterval = PERIOD_100ms;
  commsT_::SEND_TO_BOARD_INTERVAL_LOCAL = PERIOD_200ms;
  commsT_::thisTask->printSendToQueue = true;

  namespace dispt_ = DisplayTaskBase;
  dispt_::start(PERIOD_50ms);

  namespace nct_ = NintendoClassicTaskBase;
  nct_::start();
  nct_::thisTask->doWorkInterval = PERIOD_50ms;

  // mocks ---
  qwiicT_::qwiicButton.setMockIsPressedCallback([] {
    return false;
  });

  nct_::classic.setMockGetButtonEventCallback([] {
    switch (counter)
    {
    case 2:
      return (uint8_t)NintendoController::BUTTON_START;
    }
    return (uint8_t)NintendoController::BUTTON_NONE;
  });

  commsT_::boardClient.mockResponseCallback([](ControllerData out) {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = false;
    // Serial.printf("[%lu] mockMovingResponse called, version: %.1f\n", millis(), mockresp.version);
    return mockresp;
  });

  // wait for tasks ---
  while (qwiicT_::thisTask->ready == false ||
         commsT_::thisTask->ready == false ||
         nct_::thisTask->ready == false ||
         dispt_::thisTask->ready == false ||
         false)
  {
    vTaskDelay(10);
  }

  DEBUG("Tasks ready");

  // enable tasks ---
  qwiicT_::thisTask->enable(PRINT_THIS);
  commsT_::thisTask->enable(PRINT_THIS);
  nct_::thisTask->enable(PRINT_THIS);
  dispt_::thisTask->enable(PRINT_THIS);

  vTaskDelay(PERIOD_500ms);

  counter = 0;

  // clear the queue
  scheduleQueue->read();

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    // send

    Serial.printf("Counter: %d\n", counter);

    counter++;

    vTaskDelay(TICKS_1s);
  }

  vTaskDelay(PERIOD_1s);

  // OrchestratorTask::thisTask->deleteTask();
  qwiicT_::thisTask->deleteTask();
  commsT_::thisTask->deleteTask();
  dispt_::thisTask->deleteTask();
  nct_::thisTask->deleteTask();

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
  // RUN_TEST(OrchestratorTask_usesBroadcastToGetResponses_getsResponseswhenRequested);
  RUN_TEST(DisplayTask_stuff);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
