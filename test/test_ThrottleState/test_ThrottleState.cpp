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

#include <types.h>
#include <rtosManager.h>
// #include <QueueManager.h>
#include <QueueManager1.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>

#include <types/SendToBoardNotf.h>

#include <MockMagThrottle.h>
#include <MockedQwiicButton.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

xQueueHandle xBoardPacketQueue;
xQueueHandle xSendToBoardQueueHandle;
xQueueHandle xBoardStateQueueHandle;
xQueueHandle xPrimaryButtonQueue;
xQueueHandle xStatsQueue;
xQueueHandle xNintendoControllerQueue;
xQueueHandle xThrottleQueue;

//----------------------------------
#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

#define TICKS_5ms 5 / portTICK_PERIOD_MS
#define TICKS_10ms 10 / portTICK_PERIOD_MS
#define TICKS_50ms 50 / portTICK_PERIOD_MS
#define TICKS_100ms 100 / portTICK_PERIOD_MS

#include <tasks/core0/QwiicButtonTask.h>
#include <tasks/core0/ThrottleTask.h>
#include <tasks/core0/SendToBoardTimerTask.h>

elapsedMillis since_checked_queue;

const unsigned long SECONDS = 1000;

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
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf));
  xPrimaryButtonQueue = xQueueCreate(1, sizeof(PrimaryButtonState));
  xThrottleQueue = xQueueCreate(1, sizeof(ThrottleState));
}

void tearDown()
{
}

#define CORE_0 0
#define PRIORITY_1 1

void test_throttle_state_queue()
{
  namespace throttle_ = ThrottleTask;
  namespace sendNotf_ = SendToBoardTimerTask;

  Wire.begin(); //Join I2C bus

  throttle_::mgr.create(throttle_::task, CORE_0, PRIORITY_1);
  sendNotf_::mgr.create(sendNotf_::task, CORE_0, TASK_PRIORITY_2);

  Queue1::Manager<SendToBoardNotf> sendToBoardNotf(xSendToBoardQueueHandle, (TickType_t)5);
  sendNotf_::mgr.enable();

  Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueue, (TickType_t)5);
  Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueue, (TickType_t)5);

  static uint8_t _throttle = 127;

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _throttle - 1;
    return _throttle;
  });

  while (!throttle_::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test the throttle state queue");

  elapsedMillis since_checked_queue;

  PrimaryButtonState state;

  counter = 0;

  while (counter < 6)
  {
    if (since_checked_queue > 10)
    {
      since_checked_queue = 0;
      if (sendToBoardNotf.hasValue())
      {
        state.pressed = false;
        primaryButtonQueue.send(&state, [](PrimaryButtonState state) {
          // Serial.printf("sending primary button: %d\n", state.pressed);
        });

        vTaskDelay(50);

        if (throttleQueue.hasValue("test::throttleQueue (hasValue)"))
        {
          // Serial.printf("Got %d from ThrottleTask\n", throttleQueue.value.val);
          TEST_ASSERT_EQUAL(_throttle, throttleQueue.value.val);
        }

        counter++;
      }
    }
    vTaskDelay(100);
  }
}

void test_throttle_limting_with_primary_button()
{
  namespace throttle_ = ThrottleTask;
  namespace qwiic_ = QwiicButtonTask;

  Wire.begin(); //Join I2C bus

  throttle_::mgr.create(throttle_::task, CORE_0, PRIORITY_1);
  qwiic_::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);

  Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueue, (TickType_t)5);
  Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueue, (TickType_t)5);

  static uint8_t _s_Throttle = 127;
  static uint8_t _s_Qwiic_pressed = 0;

  qwiic_::qwiicButton.setMockIsPressedCallback([] {
    _s_Qwiic_pressed = counter >= 3;
    return _s_Qwiic_pressed == 1;
  });

  static uint8_t _s_Steps[] = {127, 140, 130, 127, 140, 130};

  MagneticThrottle::setGetThrottleCb([] {
    _s_Throttle = _s_Steps[counter];
    return _s_Throttle;
  });

  while (!throttle_::mgr.ready && !qwiic_::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test the throttle limiting (by primaryButton)");

  elapsedMillis since_checked_queue;

  PrimaryButtonState state;

  counter = 0;

  while (counter < 6)
  {
    if (since_checked_queue > 1000)
    {
      since_checked_queue = 0;

      state.pressed = _s_Qwiic_pressed;

      // primaryButtonQueue.send(&state, [](PrimaryButtonState state) {
      //   Serial.printf("Sending to PrimaryButtonQueue: %d\n", state.pressed);
      // });

      vTaskDelay(50);

      // if (throttleQueue.hasValue(__func__))
      // {
      //   Serial.printf("Got %d from ThrottleTask\n", throttleQueue.value.val);
      //   // if (_s_Qwiic_pressed == 0)
      //   // {
      //   //   TEST_ASSERT_TRUE(true);
      //   // }
      //   // else
      //   // {
      //   //   TEST_ASSERT_TRUE(false);
      //   // }
      //   // TEST_ASSERT_EQUAL(127, throttleQueue.value.val);
      //   // TEST_ASSERT_EQUAL(_s_Steps[counter], throttleQueue.value.val);
      // }
      Serial.printf("test 1\n");
      counter++;
      Serial.printf("test 2\n");
      vTaskDelay(5);
    }
    vTaskDelay(100);
  }

  vTaskDelete(NULL);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  RUN_TEST(test_throttle_state_queue);
  // RUN_TEST(test_throttle_limting_with_primary_button);

  UNITY_END();
}

void loop()
{
}
