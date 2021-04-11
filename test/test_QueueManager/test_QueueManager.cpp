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

QueueHandle_t xFirstQueueHandle;
QueueHandle_t xOtherTestQueueHandle;

RTOSTaskManager firstTask("FirstTask", 3000);
RTOSTaskManager otherTask("OtherTask", 3000);

class FirstTestObj : public QueueBase
{
public:
  FirstTestObj() : QueueBase(event_id)
  {
  }

  uint16_t firstvalue;
  unsigned long event_id = 0;
};

class OtherTestObj : public QueueBase
{
public:
  OtherTestObj() : QueueBase(event_id)
  {
  }

  uint16_t othervalue;
  unsigned long event_id;
};

//----------------------------------
// #include <tasks/core0/QwiicButtonTask.h>
// #include <tasks/core0/ThrottleTask.h>
// #include <tasks/core0/SendToBoardTimerTask.h>

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
  xFirstQueueHandle = xQueueCreate(1, sizeof(FirstTestObj));
  xOtherTestQueueHandle = xQueueCreate(1, sizeof(OtherTestObj));
}

void tearDown()
{
}

#define CORE_0 0
#define PRIORITY_1 1

void test_calls_and_responds_in_same_task()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, (TickType_t)5, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, (TickType_t)5, "readQueue");

  printTestInstructions("Test calls and responds in same task");

  elapsedMillis since_checked_queue, since_last_created;

  FirstTestObj firstObj;
  OtherTestObj otherObj;

  counter = 0;
  firstObj.event_id = 0;

  while (counter < 10)
  {
    if (since_last_created > 1000)
    {
      since_last_created = 0;
      sendQueue.send(&firstObj);

      bool gotObj = false, timedout = false;
      do
      {
        vTaskDelay(50);
        gotObj = readQueue.hasValue("readQueue.hasValue()");
        timedout = since_last_created > 500;
      } while (!gotObj && !timedout);

      TEST_ASSERT_FALSE(timedout);
      TEST_ASSERT_TRUE(gotObj);

      counter++;
      Serial.printf("counter: %d\n", counter);
    }

    vTaskDelay(100);
  }
  TEST_ASSERT_TRUE(counter >= 10);
}

void taskSend(void *pvParameters)
{
  DEBUG("taskSend created!");

  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, (TickType_t)5, "sendQueue");
  DEBUG("taskSend ready!");

  elapsedMillis since_sent;

  FirstTestObj sentObj;
  // sentObj.event_id = 0;

  while (true)
  {
    if (since_sent > 1000)
    {
      since_sent = 0;

      sendQueue.send(&sentObj);
    }

    vTaskDelay(10);
  }

  vTaskDelete(NULL);
}

void test_calls_from_one_task_and_reads_in_another()
{
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, (TickType_t)5, "readQueue");

  printTestInstructions("Test calls from one task and reads in another");

  FirstTestObj firstObj;
  OtherTestObj otherObj;

  xTaskCreate(taskSend, "taskSend", /*stack*/ 3000, NULL, /*priority*/ 1, CORE_0);

  counter = 0;
  firstObj.event_id = 0;

  elapsedMillis since_last_packet = 0;

  while (counter < 10)
  {
    bool gotObj = false, timedout = false;
    do
    {
      vTaskDelay(50);
      gotObj = readQueue.hasValue("readQueue.hasValue()");
      if (gotObj)
        since_last_packet = 0;
      timedout = since_last_packet > 1000;
    } while (!gotObj && !timedout);

    TEST_ASSERT_FALSE(timedout);
    TEST_ASSERT_TRUE(gotObj);

    counter++;
    Serial.printf("counter: %d\n", counter);

    vTaskDelay(100);
  }

  TEST_ASSERT_TRUE(counter >= 10);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  // RUN_TEST(test_calls_and_responds_in_same_task);
  RUN_TEST(test_calls_from_one_task_and_reads_in_another);

  UNITY_END();
}

void loop()
{
}
