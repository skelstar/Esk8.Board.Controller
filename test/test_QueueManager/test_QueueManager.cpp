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
#include <testUtils.h>

#include <types/SendToBoardNotf.h>

QueueHandle_t xFirstQueueHandle;
QueueHandle_t xOtherTestQueueHandle;
QueueHandle_t xSendToBoardQueueHandle;

#include <tasks/core0/SendToBoardTimerTask.h>

RTOSTaskManager firstTask("FirstTask", 3000);
RTOSTaskManager otherTask("OtherTask", 3000);

class FirstTestObj : public QueueBase
{
public:
  uint16_t firstvalue;
  unsigned long event_id = 0;
  // unsigned long latency = 0;

public:
  FirstTestObj() : QueueBase(event_id, 0)
  {
    queue_type = QueueType::QT_OTHER;
  }
};

class OtherTestObj : public QueueBase
{
public:
  OtherTestObj() : QueueBase(event_id, 0)
  {
  }

  uint16_t othervalue;
  unsigned long event_id;
};

//----------------------------------
// #include <tasks/core0/QwiicButtonTask.h>
// #include <tasks/core0/ThrottleTask.h>
// #include <tasks/core0/SendToBoardTimerTask.h>

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

  xFirstQueueHandle = xQueueCreate(1, sizeof(FirstTestObj));
  xOtherTestQueueHandle = xQueueCreate(1, sizeof(OtherTestObj));
  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf *));
}

void tearDown()
{
}

void test_calls_and_responds_in_same_task()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

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

namespace SendFirstObjOnInterval
{
  bool ready = false;

  void task(void *pvParameters)
  {
    unsigned long interval = pvParameters != nullptr
                                 ? *((unsigned long *)pvParameters)
                                 : 1000;

    Serial.printf("[TASK] SendFirstObjOnInterval_task created (interval: %lu)!\n", interval);

    Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");

    ready = true;
    DEBUG("[TASK] SendFirstObjOnInterval_task ready!");

    FirstTestObj sentObj;

    elapsedMillis since_sent = interval - 100;

    while (true)
    {
      if (since_sent > interval)
      {
        Serial.printf("---------------------\n");
        since_sent = 0;
        unsigned long last_id = sentObj.event_id;

        sendQueue.send(&sentObj);
        TEST_ASSERT_TRUE_MESSAGE(sentObj.event_id == last_id + 1, "firstObj.event_id did not get +1");
      }

      vTaskDelay(10);
    }

    vTaskDelete(NULL);
  }
}

void test_calls_from_one_task_and_reads_in_another()
{
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another");

  FirstTestObj firstObj;
  OtherTestObj otherObj;

  unsigned long sendInterval = 3000;

  xTaskCreatePinnedToCore(SendFirstObjOnInterval::task,
                          "SendFirstObjOnIntervalTask",
                          /*stack*/ 3000,
                          /*params*/ (void *)&sendInterval,
                          /*priority*/ 1,
                          /*handle*/ NULL,
                          /*CORE*/ 0);

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

void test_queue_hasValue()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another using waitForNewResponse()");

  FirstTestObj sendObj;

  counter = 0;
  sendObj.event_id = 0;

  elapsedMillis since_last_checked = 0;

  unsigned long last_event_id = -1;

  while (counter < 5)
  {
    ulong sent_event_id = sendObj.event_id;

    sendQueue.send(&sendObj);

    TEST_ASSERT_TRUE(sent_event_id == sendObj.event_id - 1);
    DEBUG("PASS: event_id incremented");

    vTaskDelay(5);

    FirstTestObj *res = readQueue.peek(__func__);
    TEST_ASSERT_NOT_NULL(res);
    DEBUG("PASS: peek is NOT NULL");

    TEST_ASSERT_TRUE(readQueue.hasValue());
    DEBUG("PASS: queue hasValue()");

    TEST_ASSERT_FALSE(readQueue.hasValue());
    DEBUG("PASS: queue does NOT hasValue() the second time");

    counter++;
    vTaskDelay(500);
  }

  TEST_ASSERT_TRUE(counter >= 5);
}

void testUtils_waitForNewResponse()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another using waitForNewResponse()");

  FirstTestObj sendObj;

  unsigned long sendInterval = 1 * SECONDS;

  counter = 0;
  sendObj.event_id = 0;

  elapsedMillis since_last_packet = 0,
                since_last_checked = 0;
  unsigned long last_event_id = -1;

  while (counter < 5)
  {
    ulong sent_event_id = sendObj.event_id;
    sendQueue.send(&sendObj);

    TEST_ASSERT_TRUE(sent_event_id == sendObj.event_id - 1);
    DEBUG("PASS: event_id incremented");

    vTaskDelay(5);

    FirstTestObj *res = readQueue.peek(__func__);

    TEST_ASSERT_NOT_NULL(res);
    DEBUG("PASS: peek is NOT NULL");

    uint8_t resp = waitForNew(&readQueue, 2 * SECONDS, QueueBase::printRead); // ::waitForNewResponse(readQueue, gotResp, timedout, 2 * 1000);

    TEST_ASSERT_TRUE(resp == Response::OK);
    DEBUG("PASS: waiting found packet");

    counter++;
    vTaskDelay(500);
  }

  TEST_ASSERT_TRUE(counter >= 5);
}

void test_queue_hasEvent_updates_id()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another using waitForNewResponse()");

  FirstTestObj firstObj;
  OtherTestObj otherObj;

  counter = 0;
  firstObj.event_id = 0;
  unsigned long og_event_id = -1;

  elapsedMillis since_last_packet = 0;

  while (counter < 10)
  {
    since_last_packet = 0;

    og_event_id = firstObj.event_id;
    sendQueue.send(&firstObj);
    // test to see that event_id got bumped by +1
    TEST_ASSERT_TRUE_MESSAGE(firstObj.event_id == og_event_id + 1, "firstObj.event_id did not +1");

    // test hasValue
    bool hasValue = readQueue.hasValue();
    TEST_ASSERT_TRUE_MESSAGE(hasValue, "readQueue did not find a message!");

    // test reading it twice equals false
    bool hasValueAgain = readQueue.hasValue();
    TEST_ASSERT_FALSE_MESSAGE(hasValueAgain, "readQueue found the message of same value twice!");

    counter++;
    Serial.printf("counter: %d\n", counter);

    vTaskDelay(500);
  }

  TEST_ASSERT_TRUE(counter >= 10);
}

void testUtils_waitForNewResp_with_QueueType()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another using waitForNewResponse()");

  FirstTestObj sendObj;
  sendObj.queue_type = QueueType::QT_Notification;

  counter = 0;
  sendObj.event_id = 0;

  while (counter < 5)
  {
    DEBUG("--------------------------");

    ulong sent_event_id = sendObj.event_id;

    // send
    sendQueue.send_r(&sendObj, QueueBase::printSend);

    TEST_ASSERT_TRUE(sent_event_id == sendObj.event_id - 1);
    DEBUG("PASS: event_id incremented");
    DEBUGVAL(sent_event_id, sendObj.event_id);

    // read/wait for new
    uint8_t resp = waitForNew(&readQueue, PERIOD_100ms, QueueBase::printRead);
    TEST_ASSERT_TRUE(resp == Response::OK);
    DEBUG("PASS: waiting found packet");

    resp = waitForNew(&readQueue, 100 * MILLIS_S, QueueBase::printRead);
    TEST_ASSERT_TRUE(resp == Response::TIMEOUT);
    DEBUG("PASS: waiting didn't find new packet");

    counter++;
    vTaskDelay(200);
  }

  TEST_ASSERT_TRUE(counter >= 5);
}

void testUtils_waitForNewResp_with_QueueType_from_Notification_task()
{
  Queue1::Manager<SendToBoardNotf> *readNotfQueue = new Queue1::Manager<SendToBoardNotf>(xSendToBoardQueueHandle, TICKS_5ms, "(test)readNotfQueue");

  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);

  SendToBoardTimerTask::setSendInterval(3 * SECONDS);

  printTestInstructions("Test testUtils_waitForNewResp_with_QueueType_from_Notification_task");

  while (SendToBoardTimerTask::mgr.ready == false)
    vTaskDelay(50);

  SendToBoardTimerTask::mgr.enable();

  counter = 0;

  while (counter < 5)
  {
    DEBUG("--------------------------");

    uint8_t firstResp = waitForNew(readNotfQueue, 10 * SECONDS, QueueBase::printRead);

    uint8_t secondResp = waitForNew(readNotfQueue, PERIOD_50ms, QueueBase::printRead);

    TEST_ASSERT_TRUE_MESSAGE(firstResp == Response::OK, "firstResp was not OK");
    TEST_ASSERT_TRUE_MESSAGE(secondResp == Response::TIMEOUT, "secondResp was not TIMEOUT");
    DEBUG("PASS: found Notf and didn't find it again");

    counter++;
    vTaskDelay(200);
  }

  TEST_ASSERT_TRUE(counter >= 5);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  // RUN_TEST(test_calls_and_responds_in_same_task);
  // RUN_TEST(test_calls_from_one_task_and_reads_in_another);
  // RUN_TEST(test_queue_hasValue);
  // RUN_TEST(testUtils_waitForNewResponse);
  // RUN_TEST(test_queue_hasEvent_updates_id);
  // RUN_TEST(testUtils_waitForNewResp_with_QueueType);
  RUN_TEST(testUtils_waitForNewResp_with_QueueType_from_Notification_task);

  UNITY_END();
}

void loop()
{
}
