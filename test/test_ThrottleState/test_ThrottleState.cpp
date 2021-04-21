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
#include <QueueManager1.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>
// #include <testUtils.h>
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

//----------------------------------
#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

static int counter = 0;

elapsedMillis since_checked_queue;

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

namespace qwiicT_ = QwiicTaskBase;
namespace commsT_ = BoardCommsTask;
namespace dispt_ = DisplayTaskBase;
namespace nct_ = NintendoClassicTaskBase;
namespace throttle_ = ThrottleTaskBase;

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

  /* #region  tasks */

  qwiicT_::start(/*work*/ PERIOD_100ms);
  commsT_::start(/*work*/ PERIOD_100ms, /*send*/ PERIOD_200ms);
  nct_::start(/*work*/ PERIOD_50ms);
  dispt_::start(/*work*/ PERIOD_50ms);
  ThrottleTaskBase::start(/*work*/ PERIOD_200ms);

  /* #endregion */

  counter = 0;
}

void tearDown()
{
  qwiicT_::thisTask->deleteTask();
  commsT_::thisTask->deleteTask();
  dispt_::thisTask->deleteTask();
  nct_::thisTask->deleteTask();
  ThrottleTaskBase::thisTask->deleteTask();
}

VescData mockBoardStoppedResponse(ControllerData out)
{
  VescData mockresp;
  mockresp.id = out.id;
  mockresp.version = VERSION_BOARD_COMPAT;
  mockresp.moving = false;
  // Serial.printf("[%lu] mockMovingResponse called, moving: %d\n", millis(), mockresp.moving);
  return mockresp;
}

void ThrottleZero_boardReportsStopped()
{
  static uint8_t _throttle = 127;

  /* #region  Mocks */
  MagneticThrottle::setGetThrottleCb([] {
    _throttle = 127;
    return _throttle;
  });
  MagneticThrottle::setThrottleEnabledCb([] {
    return true;
  });
  qwiicT_::qwiicButton.setMockIsPressedCallback([] {
    return false;
  });
  commsT_::boardClient.mockResponseCallback(mockBoardStoppedResponse);

  /* #endregion */

  while (
      qwiicT_::thisTask->ready == false ||
      commsT_::thisTask->ready == false ||
      nct_::thisTask->ready == false ||
      dispt_::thisTask->ready == false ||
      throttle_::thisTask->ready == false)
    vTaskDelay(TICKS_5ms);

  /* #region  enable tasks */
  qwiicT_::thisTask->enable(PRINT_THIS);
  commsT_::thisTask->enable(PRINT_THIS);
  nct_::thisTask->enable(PRINT_THIS);
  dispt_::thisTask->enable(PRINT_THIS);
  throttle_::thisTask->enable();

  Serial.printf("%lums\n", millis());
  vTaskDelay(TICKS_1s);
  Serial.printf("%lums\n", millis());
  /* #endregion */

  TEST_ASSERT_TRUE_MESSAGE(throttleQueue->hasValue(), "throttleQueue does not have a value");
  TEST_ASSERT_TRUE_MESSAGE(throttleQueue->payload.val == 127, "throttleQueue value is not 127");
}

void test_throttle_limting_with_primary_button_not_held()
{

  // Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  // // Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");

  // static uint8_t _throttle = 127;
  // static uint8_t _s_Steps[] = {50, 60, 120, 127, 130, 140};

  // MagneticThrottle::setGetThrottleCb([] {
  //   _throttle = _s_Steps[counter];
  //   return _throttle;
  // });

  // while (!throttle_::thisTask->ready)
  // {
  //   vTaskDelay(5);
  // }

  // printTestInstructions("Will test the throttle stays <=127 when primary button not held");

  // elapsedMillis since_checked_queue;

  // counter = 0;
  // Serial.printf("------------------------------------\n");

  // while (counter < 6)
  // {
  //   if (since_checked_queue > 10)
  //   {
  //     since_checked_queue = 0;
  //     if (sendNotfQueue.hasValue())
  //     {
  //       bool gotResp = false, timedout = false;
  //       do
  //       {
  //         gotResp = throttleQueue.hasValue();
  //         if (gotResp)
  //         {
  //           since_checked_queue = 0;
  //           Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
  //         }
  //         timedout = since_checked_queue > 500;
  //         vTaskDelay(5);
  //       } while (!gotResp && !timedout);

  //       TEST_ASSERT_FALSE(timedout);
  //       TEST_ASSERT_TRUE(gotResp);
  //       uint8_t expected = _s_Steps[counter] <= 127 ? _s_Steps[counter] : 127;
  //       TEST_ASSERT_EQUAL(expected, throttleQueue.payload.val);
  //       Serial.printf("------------------------------------\n");

  //       counter++;
  //     }
  //   }
  //   vTaskDelay(100);
  // }

  // TEST_ASSERT_TRUE(counter == 6);
}

void test_throttle_not_limting_when_primary_button_is_held()
{
  static uint8_t _throttle = 127;
  static uint8_t _s_Steps[] = {50, 60, 120, 127, 130, 140};

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _s_Steps[counter];
    return _throttle;
  });

  while (!throttle_::thisTask->ready)
  {
    vTaskDelay(5);
  }

  PrimaryButtonState state;
  state.pressed = 1;

  primaryButtonQueue->send(&state); //, printSentToQueue);

  printTestInstructions("Tests the throttle stays normal when primary button is held");

  elapsedMillis since_checked_queue;

  counter = 0;
  Serial.printf("------------------------------------\n");

  while (counter < 6)
  {
    // if (since_checked_queue > 10)
    // {
    //   since_checked_queue = 0;
    //   if (sendNotfQueue.hasValue())
    //   {
    //     bool gotResp = false, timedout = false;
    //     do
    //     {
    //       gotResp = throttleQueue.hasValue();
    //       if (gotResp)
    //       {
    //         since_checked_queue = 0;
    //         Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
    //       }
    //       timedout = since_checked_queue > 500;
    //       vTaskDelay(5);
    //     } while (!gotResp && !timedout);

    //     TEST_ASSERT_FALSE(timedout);
    //     TEST_ASSERT_TRUE(gotResp);
    //     uint8_t expected = _s_Steps[counter];
    //     TEST_ASSERT_EQUAL(expected, throttleQueue.payload.val);
    //     Serial.printf("------------------------------------\n");

    //     counter++;
    //   }
    // }
    vTaskDelay(100);
  }

  TEST_ASSERT_TRUE(counter == 6);
}

void test_throttle_limts_then_does_not_limit()
{
  // Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  // Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");

  // static uint8_t _throttle = 127;
  // static uint8_t _s_ThrottleSteps[] = {88, 130, 140, 127, 130, 140};
  // static uint8_t _s_PrimarySteps[]{0, 0, 0, 1, 1, 1};

  // MagneticThrottle::setGetThrottleCb([] {
  //   _throttle = _s_ThrottleSteps[counter];
  //   return _throttle;
  // });

  // while (!throttle_::thisTask->ready)
  // {
  //   vTaskDelay(5);
  // }

  // PrimaryButtonState state;
  // state.pressed = 1;

  // primaryButtonQueue.send(&state); //, printSentToQueue);

  // printTestInstructions("Tests the throttle stays normal when primary button is held");

  // elapsedMillis since_checked_queue;

  // counter = 0;
  // Serial.printf("------------------------------------\n");

  // while (counter < 6)
  // {

  //   since_checked_queue = 0;
  //   if (sendNotfQueue.hasValue())
  //   {
  //     state.pressed = _s_PrimarySteps[counter];
  //     primaryButtonQueue.send(&state);

  //     vTaskDelay(50);

  //     bool gotResp = false, timedout = false;
  //     do
  //     {
  //       gotResp = throttleQueue.hasValue();
  //       if (gotResp)
  //       {
  //         since_checked_queue = 0;
  //         Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
  //       }
  //       timedout = since_checked_queue > 500;
  //       vTaskDelay(5);
  //     } while (!gotResp && !timedout);

  //     TEST_ASSERT_FALSE(timedout);
  //     TEST_ASSERT_TRUE(gotResp);
  //     uint8_t expected = !state.pressed && _s_ThrottleSteps[counter] > 127
  //                            ? 127
  //                            : _s_ThrottleSteps[counter];
  //     TEST_ASSERT_EQUAL(expected, throttleQueue.payload.val);
  //     Serial.printf("------------------------------------\n");

  //     counter++;
  //   }
  //   vTaskDelay(100);
  // }

  // TEST_ASSERT_TRUE(counter == 6);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  RUN_TEST(ThrottleZero_boardReportsStopped);
  // RUN_TEST(test_ThrottleTask_sends_when_SendToBoardTask_triggers);
  // RUN_TEST(test_throttle_limting_with_primary_button_not_held);
  // RUN_TEST(test_throttle_not_limting_when_primary_button_is_held);
  // RUN_TEST(test_throttle_limts_then_does_not_limit);

  UNITY_END();
}

void loop()
{
}
