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
#include <types/PrimaryButton.h>
#include <types/Throttle.h>

#include <MockMagThrottle.h>
#include <MockQwiicButton.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

xQueueHandle xBoardPacketQueue;
xQueueHandle xSendToBoardQueueHandle;
xQueueHandle xPacketStateQueueHandle;
xQueueHandle xPrimaryButtonQueueHandle;
xQueueHandle xStatsQueue;
xQueueHandle xNintendoControllerQueue;
xQueueHandle xThrottleQueueHandle;

//----------------------------------
#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

#include <tasks/core0/QwiicButtonTask.h>
#include <tasks/core0/ThrottleTask.h>
#include <tasks/core0/OrchestratorTask.h>

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

void setUp()
{
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState));
}

void tearDown()
{
}

void test_ThrottleTask_sends_when_SendToBoardTask_triggers()
{
  namespace throttleTask = ThrottleTask;
  namespace sendNotfTask = OrchestratorTask;

  Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  // Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");
  Queue1::Manager<SendToBoardNotf> sendNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, "sendNotfQueue");

  Wire.begin(); //Join I2C bus

  throttleTask::mgr.create(throttleTask::task, CORE_0, TASK_PRIORITY_1);
  sendNotfTask::mgr.create(sendNotfTask::task, CORE_0, TASK_PRIORITY_2);

  sendNotfTask::setSendInterval(1000);

  sendNotfTask::mgr.enable();

  static uint8_t _throttle = 127;

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _throttle - 1;
    return _throttle;
  });

  while (!throttleTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test the throttle state queue sends out when sendBoardNotification is sent");

  elapsedMillis since_checked_queue;

  PrimaryButtonState state;

  counter = 0;
  Serial.printf("------------------------------------\n");

  while (counter < 6)
  {
    if (since_checked_queue > 10)
    {
      since_checked_queue = 0;
      if (sendNotfQueue.hasValue())
      {
        bool gotResp = false, timedout = false;
        do
        {
          gotResp = throttleQueue.hasValue();
          if (gotResp)
          {
            since_checked_queue = 0;
            Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
          }
          timedout = since_checked_queue > 500;
          vTaskDelay(5);
        } while (!gotResp && !timedout);

        TEST_ASSERT_FALSE(timedout);
        TEST_ASSERT_TRUE(gotResp);
        Serial.printf("------------------------------------\n");

        counter++;
      }
    }
    vTaskDelay(100);
  }

  throttleTask::mgr.deleteTask(PRINT_THIS);
  sendNotfTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter == 6);
}

void test_throttle_limting_with_primary_button_not_held()
{
  namespace throttleTask = ThrottleTask;
  namespace sendNotfTask = OrchestratorTask;

  Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  Queue1::Manager<SendToBoardNotf> sendNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, "sendNotfQueue");
  // Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");

  throttleTask::mgr.create(throttleTask::task, CORE_0, TASK_PRIORITY_1);
  sendNotfTask::mgr.create(sendNotfTask::task, CORE_0, TASK_PRIORITY_2);

  sendNotfTask::setSendInterval(500);

  sendNotfTask::mgr.enable();

  static uint8_t _throttle = 127;
  static uint8_t _s_Steps[] = {50, 60, 120, 127, 130, 140};

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _s_Steps[counter];
    return _throttle;
  });

  while (!throttleTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test the throttle stays <=127 when primary button not held");

  elapsedMillis since_checked_queue;

  counter = 0;
  Serial.printf("------------------------------------\n");

  while (counter < 6)
  {
    if (since_checked_queue > 10)
    {
      since_checked_queue = 0;
      if (sendNotfQueue.hasValue())
      {
        bool gotResp = false, timedout = false;
        do
        {
          gotResp = throttleQueue.hasValue();
          if (gotResp)
          {
            since_checked_queue = 0;
            Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
          }
          timedout = since_checked_queue > 500;
          vTaskDelay(5);
        } while (!gotResp && !timedout);

        TEST_ASSERT_FALSE(timedout);
        TEST_ASSERT_TRUE(gotResp);
        uint8_t expected = _s_Steps[counter] <= 127 ? _s_Steps[counter] : 127;
        TEST_ASSERT_EQUAL(expected, throttleQueue.payload.val);
        Serial.printf("------------------------------------\n");

        counter++;
      }
    }
    vTaskDelay(100);
  }

  throttleTask::mgr.deleteTask(PRINT_THIS);
  sendNotfTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter == 6);
}

void test_throttle_not_limting_when_primary_button_is_held()
{
  namespace throttleTask = ThrottleTask;
  namespace sendNotfTask = OrchestratorTask;

  Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  Queue1::Manager<SendToBoardNotf> sendNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, "sendNotfQueue");
  Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");

  throttleTask::mgr.create(throttleTask::task, CORE_0, TASK_PRIORITY_1);
  sendNotfTask::mgr.create(sendNotfTask::task, CORE_0, TASK_PRIORITY_2);

  sendNotfTask::setSendInterval(500);

  sendNotfTask::mgr.enable();

  static uint8_t _throttle = 127;
  static uint8_t _s_Steps[] = {50, 60, 120, 127, 130, 140};

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _s_Steps[counter];
    return _throttle;
  });

  while (!throttleTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  PrimaryButtonState state;
  state.pressed = 1;

  Serial.printf("test 1\n");
  primaryButtonQueue.send(&state); //, printSentToQueue);
  Serial.printf("test 2\n");

  printTestInstructions("Tests the throttle stays normal when primary button is held");

  elapsedMillis since_checked_queue;

  counter = 0;
  Serial.printf("------------------------------------\n");

  while (counter < 6)
  {
    if (since_checked_queue > 10)
    {
      since_checked_queue = 0;
      if (sendNotfQueue.hasValue())
      {
        bool gotResp = false, timedout = false;
        do
        {
          gotResp = throttleQueue.hasValue();
          if (gotResp)
          {
            since_checked_queue = 0;
            Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
          }
          timedout = since_checked_queue > 500;
          vTaskDelay(5);
        } while (!gotResp && !timedout);

        TEST_ASSERT_FALSE(timedout);
        TEST_ASSERT_TRUE(gotResp);
        uint8_t expected = _s_Steps[counter];
        TEST_ASSERT_EQUAL(expected, throttleQueue.payload.val);
        Serial.printf("------------------------------------\n");

        counter++;
      }
    }
    vTaskDelay(100);
  }

  throttleTask::mgr.deleteTask(PRINT_THIS);
  sendNotfTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter == 6);
}

void test_throttle_limts_then_does_not_limit()
{
  namespace throttleTask = ThrottleTask;
  namespace sendNotfTask = OrchestratorTask;

  Queue1::Manager<ThrottleState> throttleQueue(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  Queue1::Manager<SendToBoardNotf> sendNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, "sendNotfQueue");
  Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");

  throttleTask::mgr.create(throttleTask::task, CORE_0, TASK_PRIORITY_1);
  sendNotfTask::mgr.create(sendNotfTask::task, CORE_0, TASK_PRIORITY_2);

  sendNotfTask::setSendInterval(500);

  sendNotfTask::mgr.enable();

  static uint8_t _throttle = 127;
  static uint8_t _s_ThrottleSteps[] = {88, 130, 140, 127, 130, 140};
  static uint8_t _s_PrimarySteps[]{0, 0, 0, 1, 1, 1};

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _s_ThrottleSteps[counter];
    return _throttle;
  });

  while (!throttleTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  PrimaryButtonState state;
  state.pressed = 1;

  primaryButtonQueue.send(&state); //, printSentToQueue);

  printTestInstructions("Tests the throttle stays normal when primary button is held");

  elapsedMillis since_checked_queue;

  counter = 0;
  Serial.printf("------------------------------------\n");

  while (counter < 6)
  {

    since_checked_queue = 0;
    if (sendNotfQueue.hasValue())
    {
      state.pressed = _s_PrimarySteps[counter];
      primaryButtonQueue.send(&state);

      vTaskDelay(50);

      bool gotResp = false, timedout = false;
      do
      {
        gotResp = throttleQueue.hasValue();
        if (gotResp)
        {
          since_checked_queue = 0;
          Serial.printf("Got response from throttleQueue, throttle=%d\n", throttleQueue.payload.val);
        }
        timedout = since_checked_queue > 500;
        vTaskDelay(5);
      } while (!gotResp && !timedout);

      TEST_ASSERT_FALSE(timedout);
      TEST_ASSERT_TRUE(gotResp);
      uint8_t expected = !state.pressed && _s_ThrottleSteps[counter] > 127
                             ? 127
                             : _s_ThrottleSteps[counter];
      TEST_ASSERT_EQUAL(expected, throttleQueue.payload.val);
      Serial.printf("------------------------------------\n");

      counter++;
    }
    vTaskDelay(100);
  }

  throttleTask::mgr.deleteTask(PRINT_THIS);
  sendNotfTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter == 6);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  // RUN_TEST(test_ThrottleTask_sends_when_SendToBoardTask_triggers);
  // RUN_TEST(test_throttle_limting_with_primary_button_not_held);
  // RUN_TEST(test_throttle_not_limting_when_primary_button_is_held);
  RUN_TEST(test_throttle_limts_then_does_not_limit);

  UNITY_END();
}

void loop()
{
}
