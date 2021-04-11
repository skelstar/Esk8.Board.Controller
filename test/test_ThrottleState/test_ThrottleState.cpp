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
#include <QueueManager2.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>

#include <types/SendToBoardNotf.h>
#include <types/PrimaryButton.h>
#include <types/Throttle.h>

#include <MockMagThrottle.h>
#include <MockedQwiicButton.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

// xQueueHandle xBoardPacketQueue;
xQueueHandle xSendToBoardQueueHandle;
// xQueueHandle xBoardStateQueueHandle;
xQueueHandle xPrimaryButtonQueueHandle;
// xQueueHandle xStatsQueue;
// xQueueHandle xNintendoControllerQueue;
xQueueHandle xThrottleQueueHandle;

Queue2::Manager<ThrottleState> throttleQueue;
Queue2::Manager<PrimaryButtonState> primaryButtonQueue;
Queue2::Manager<SendToBoardNotf> sendNotfQueue;

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
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState));

  throttleQueue.create(xThrottleQueueHandle, TICKS_10ms, "(test)ThrottleQueue");
  primaryButtonQueue.create(xPrimaryButtonQueueHandle, TICKS_5ms, "(test)PrimButtonQueue");
  sendNotfQueue.create(xSendToBoardQueueHandle, TICKS_5ms, "sendNotfQueue");
}

void tearDown()
{
}

void test_ThrottleTask_sends_when_SendToBoardTask_triggers()
{
  namespace throttleTask = ThrottleTask;
  namespace sendNotfTask = SendToBoardTimerTask;

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

  throttleTask::mgr.deleteTask(PRINT_THIS); // .create(throttleTask::task, CORE_0, TASK_PRIORITY_1);
  sendNotfTask::mgr.deleteTask(PRINT_THIS); //.create(sendNotfTask::task, CORE_0, TASK_PRIORITY_2);

  TEST_ASSERT_TRUE(counter == 6);
}

void test_throttle_limting_with_primary_button()
{
  namespace throttleTask = ThrottleTask;
  namespace sendNotfTask = SendToBoardTimerTask;

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

  throttleTask::mgr.deleteTask(PRINT_THIS); // .create(throttleTask::task, CORE_0, TASK_PRIORITY_1);
  sendNotfTask::mgr.deleteTask(PRINT_THIS); //.create(sendNotfTask::task, CORE_0, TASK_PRIORITY_2);

  TEST_ASSERT_TRUE(counter == 6);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  RUN_TEST(test_ThrottleTask_sends_when_SendToBoardTask_triggers);
  RUN_TEST(test_throttle_limting_with_primary_button);

  UNITY_END();
}

void loop()
{
}
