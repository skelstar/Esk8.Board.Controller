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

#include <types/PacketState.h>
#include <types/SendToBoardNotf.h>
#include <types/NintendoButtonEvent.h>
#include <types/PrimaryButton.h>
#include <types/Throttle.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <MockGenericClient.h>
// #include <GenericClient.h>

#define RADIO_OBJECTS
// NRF24L01Lib nrf24;

// RF24 radio(NRF_CE, NRF_CS);
// RF24Network network(radio);
GenericClient<ControllerData, VescData> boardClient(01);

#include <MockedQwiicButton.h>
#include <MockNintendoController.h>

// RTOS ENTITES-------------------

QueueHandle_t xFirstQueueHandle;
QueueHandle_t xOtherTestQueueHandle;

QueueHandle_t xBoardPacketQueue;
QueueHandle_t xNintendoControllerQueue;
QueueHandle_t xPacketStateQueueHandle;
QueueHandle_t xPrimaryButtonQueueHandle;
QueueHandle_t xSendToBoardQueueHandle;
QueueHandle_t xThrottleQueueHandle;

MyMutex mutex_I2C;
MyMutex mutex_SPI;

SemaphoreHandle_t i2cMutex;

#include <displayState.h>

// TASKS ------------------------

#include <tasks/core0/SendToBoardTimerTask.h>
#include <tasks/core0/DisplayTask.h>
#include <tasks/core0/QwiicButtonTask.h>
#include <tasks/core0/ThrottleTask.h>
#include <tasks/core0/BoardCommsTask.h>
#include <tasks/core0/NintendoClassicTask.h>
#include <tasks/core0/remoteTask.h>

RTOSTaskManager firstTask("FirstTask", 3000);
RTOSTaskManager otherTask("OtherTask", 3000);

class FirstTestObj : public QueueBase
{
public:
  uint16_t firstvalue;

public:
  FirstTestObj() : QueueBase()
  {
    name = "FirstObj";
  }
};

class OtherTestObj : public QueueBase
{
public:
  uint16_t othervalue;

public:
  OtherTestObj() : QueueBase()
  {
    name = "OtherObj";
  }
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

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState));
  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState));
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
//-----------------------------

namespace OtherObj
{
  RTOSTaskManager mgr("OtherObjTask_OG", 5000);

  void taskOtherdObj(void *pvParameters)
  {
    mgr.printStarted();

    OtherTestObj otherObj;

    Queue1::Manager<SendToBoardNotf> readNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, "(IRL)readNotf");
    Queue1::Manager<OtherTestObj> sendOtherQueue(xOtherTestQueueHandle, TICKS_5ms, "(IRL)sendOther");

    TEST_ASSERT_EQUAL_MESSAGE(((QueueBase)otherObj).event_id, otherObj.event_id, "Base does not have same id");
    DEBUG("PASS: event_ids match from otherObj.event_id and base");

    mgr.printReady();

    elapsedMillis since_ready = 0;
    while (since_ready > 100)
    {
    }

    mgr.ready = true;

    elapsedMillis since_check_for_notf = 0;

    while (true)
    {
      if (since_check_for_notf > 100 && mgr.enabled())
      {
        since_check_for_notf = 0;

        uint8_t response = waitForNew(&readNotfQueue, 100, QueueBase::printRead);
        if (response == Response::OK)
        {
          sendOtherQueue.send_r(&otherObj, QueueBase::printSend);
        }
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
}

void testUtils_waitForNewResp_with_notf_task_and_respond_task()
{
  Queue1::Manager<SendToBoardNotf> *readNotfQueue = new Queue1::Manager<SendToBoardNotf>(xSendToBoardQueueHandle, TICKS_5ms, "(test)readNotfQueue");
  Queue1::Manager<OtherTestObj> *readOtherQueue = new Queue1::Manager<OtherTestObj>(xOtherTestQueueHandle, TICKS_5ms, "(test)readOtherQueue");

  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  SendToBoardTimerTask::setSendInterval(3 * SECONDS);

  OtherObj::mgr.create(OtherObj::taskOtherdObj, 0, 1);

  printTestInstructions("testUtils_waitForNewResp_with_notf_task_and_respond_task");

  while (SendToBoardTimerTask::mgr.ready == false ||
         OtherObj::mgr.ready == false)
    vTaskDelay(50);

  OtherObj::mgr.enable();
  SendToBoardTimerTask::mgr.enable();

  counter = 0;

  while (counter < 5)
  {
    DEBUG("--------------------------");

    uint8_t firstResp = waitForNew(readNotfQueue, 10 * SECONDS);
    TEST_ASSERT_TRUE_MESSAGE(firstResp == Response::OK, "firstResp was not OK");
    DEBUG("PASS: found Notf");

    uint8_t secondResp = waitForNew(readOtherQueue, PERIOD_100ms, QueueBase::printRead);
    TEST_ASSERT_TRUE_MESSAGE(secondResp == Response::OK, "otherResp timed out after 100ms");
    DEBUG("PASS: ...and otherObjTask responded");

    counter++;
    vTaskDelay(200);
  }

  OtherObj::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= 5);
}

namespace OtherTask
{
  // RTOSTaskManager mgr("OtherTask", 5000);

  void taskOther(void *pvParameters)
  {
    Serial.printf("Task: %s started\n", (const char *)pvParameters);

    OtherTestObj otherObj;

    char queueName[30];
    sprintf(queueName, "(IRL)%s", (const char *)pvParameters);

    Queue1::Manager<SendToBoardNotf> readNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, queueName);
    Queue1::Manager<OtherTestObj> sendOtherQueue(xOtherTestQueueHandle, TICKS_5ms, queueName);

    Serial.printf("Task: %s ready\n", (const char *)pvParameters);

    elapsedMillis since_ready = 0;

    while (since_ready > 100)
      vTaskDelay(5);

    elapsedMillis since_check_for_notf = 0;

    while (true)
    {
      if (since_check_for_notf > 100)
      {
        since_check_for_notf = 0;

        uint8_t response = waitForNew(&readNotfQueue, 100, QueueBase::printRead);
        if (response == Response::OK)
          sendOtherQueue.send_r(&otherObj); //, QueueBase::printSend);
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
}

void boxing_tasks()
{
  Queue1::Manager<SendToBoardNotf> *readNotfQueue = new Queue1::Manager<SendToBoardNotf>(xSendToBoardQueueHandle, TICKS_5ms, "(test)readNotfQueue");
  Queue1::Manager<OtherTestObj> *readOtherQueue = new Queue1::Manager<OtherTestObj>(xOtherTestQueueHandle, TICKS_5ms, "(test)readOtherQueue");

  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  SendToBoardTimerTask::setSendInterval(3 * SECONDS);

#define NUM_OTHER_TASKS 3

  int t = 0;
  RTOSTaskManager *mgrs[NUM_OTHER_TASKS];

  for (RTOSTaskManager *m : mgrs)
  {
    char taskName[20];
    sprintf(taskName, "OtherTask#%d", t);

    m = new RTOSTaskManager(taskName, 5000);

    xTaskCreatePinnedToCore(OtherTask::taskOther,
                            taskName,
                            /*stack*/ 3000,
                            /*params*/ (void *)&taskName,
                            /*priority*/ t,
                            /*handle*/ NULL,
                            /*CORE*/ 0);
    t++;

    vTaskDelay(50);

    m->enable(true, PRINT_THIS);
  }

  printTestInstructions("boxing_tasks");

  SendToBoardTimerTask::mgr.enable();

#define NUM_LOOPS 5
  counter = 0;

  while (counter < NUM_LOOPS)
  {
    DEBUG("--------------------------");

    uint8_t notfResp = waitForNew(readNotfQueue, 10 * SECONDS);
    TEST_ASSERT_TRUE_MESSAGE(notfResp == Response::OK, "notfResp was not OK");

    uint8_t secondResp = waitForNew(readOtherQueue, 10 * SECONDS, QueueBase::printRead);
    TEST_ASSERT_TRUE_MESSAGE(secondResp == Response::OK, "otherResp timed out after 10s");

    counter++;
    vTaskDelay(200);
  }

  for (RTOSTaskManager *m : mgrs)
    m->deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void sendOutNotification_allTasksRespondWithCorrelationId()
{
  i2cMutex = xSemaphoreCreateMutex();

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  mutex_SPI.create("SPI", /*default*/ TICKS_50ms);
  mutex_SPI.enabled = true;

  QwiicButtonTask::qwiicButton.setMockIsPressedCallback([] {
    return false;
  });

  Queue1::Manager<SendToBoardNotf> *sendNotfQueue = SendToBoardTimerTask::createQueueManager("test)sendNotfQueue");
  Queue1::Manager<PrimaryButtonState> *readPrimaryButtonQueue = QwiicButtonTask::createQueueManager("(test)readPrimaryButtonQueue");
  Queue1::Manager<ThrottleState> *readThrottleQueue = ThrottleTask::createQueueManager("(test)readThrottleQueue");
  Queue1::Manager<PacketState> *readPacketStateQueue = BoardCommsTask::createQueueManager("(test)readPacketStateQueue");
  Queue1::Manager<NintendoButtonEvent> *readNintendoQueue = NintendoClassicTask::createQueueManager("(test)readNintendoQueue");

  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  // Display::mgr.create(Display::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  QwiicButtonTask::mgr.create(QwiicButtonTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  // ThrottleTask::mgr.create(ThrottleTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  // BoardCommsTask::mgr.create(BoardCommsTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
  // NintendoClassicTask::mgr.create(NintendoClassicTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);

  SendToBoardTimerTask::setSendInterval(3 * SECONDS);

  printTestInstructions("Test testUtils_waitForNewResp_with_QueueType_from_Notification_task");

  while (SendToBoardTimerTask::mgr.ready == false ||
         //  Display::mgr.ready == false ||
         QwiicButtonTask::mgr.ready == false ||
         //  ThrottleTask::mgr.ready == false ||
         //  BoardCommsTask::mgr.ready == false
         false)
  {
    vTaskDelay(50);
  }

  DEBUG("Tasks ready!");

  SendToBoardTimerTask::mgr.enable(PRINT_THIS);

  SendToBoardNotf notification;
  notification.correlationId = 10;

  static int counter = 0;

  while (counter < 5)
  {
    DEBUG("--------------------------");

    sendNotfQueue->send_r(&notification, QueueBase::printSend);

    vTaskDelay(TICKS_1s);

    uint8_t res = waitForNew(readPrimaryButtonQueue, PERIOD_50ms);
    TEST_ASSERT_EQUAL(readPrimaryButtonQueue->payload.correlationId, /*expected*/ notification.correlationId);
    TEST_ASSERT_EQUAL(res, Response::OK);

    counter++;
    // vTaskDelay(200);
  }

  TEST_ASSERT_TRUE(counter == 5);

  QwiicButtonTask::mgr.deleteTask(PRINT_THIS);
  SendToBoardTimerTask::mgr.deleteTask(PRINT_THIS);
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
  // RUN_TEST(testUtils_waitForNewResp_with_QueueType_from_Notification_task);
  // RUN_TEST(testUtils_waitForNewResp_with_notf_task_and_respond_task);
  // RUN_TEST(boxing_tasks);
  RUN_TEST(sendOutNotification_allTasksRespondWithCorrelationId);

  UNITY_END();
}

void loop()
{
}
