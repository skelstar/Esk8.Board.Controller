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

// RTOS ENTITES-------------------

QueueHandle_t xFirstQueueHandle;
QueueHandle_t xOtherTestQueueHandle;

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

MyMutex mutex_I2C;
MyMutex mutex_SPI;

#include <types/QueueBase.h>
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

#include <MockQwiicButton.h>
#include <MockNintendoController.h>

#include <displayState.h>

// TASKS ------------------------

#include <tasks/core0/OrchestratorTask.h>
// #include <tasks/core0/DisplayTask.h>
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
// #include <tasks/core0/OrchestratorTask.h>

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

Queue1::Manager<SendToBoardNotf> *sendNotfQueue;
Queue1::Manager<PrimaryButtonState> *primaryButtonQueue;
Queue1::Manager<ThrottleState> *readThrottleQueue;
Queue1::Manager<PacketState> *readPacketStateQueue;
Queue1::Manager<NintendoButtonEvent> *readNintendoQueue;

void setUp()
{
  DEBUG("----------------------------");
  Serial.printf("    %s \n", __FILE__);
  DEBUG("----------------------------");

  xFirstQueueHandle = xQueueCreate(1, sizeof(FirstTestObj *));
  xOtherTestQueueHandle = xQueueCreate(1, sizeof(OtherTestObj *));

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));
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

  counter = 0;
  firstObj.event_id = 0;

  while (counter < 5)
  {
    if (since_last_created > 200)
    {
      Serial.printf("counter: %d\n", counter);
      since_last_created = 0;
      sendQueue.send_r(&firstObj, QueueBase::printSend);

      bool gotObj = false, timedout = false;
      do
      {
        gotObj = readQueue.hasValue();
        timedout = since_last_created > 200;
        vTaskDelay(50);
      } while (!gotObj && !timedout);

      TEST_ASSERT_FALSE_MESSAGE(timedout, "Timed out");
      TEST_ASSERT_TRUE_MESSAGE(gotObj, "Didn't get obj");

      counter++;
    }

    vTaskDelay(10);
  }
  TEST_ASSERT_TRUE(counter >= 5);
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

    DEBUG("[TASK] SendFirstObjOnInterval_task ready!");

    FirstTestObj sentObj;

    elapsedMillis since_sent = interval - 100;

    while (true)
    {
      if (since_sent > interval)
      {
        Serial.printf("---------------------\n");
        since_sent = 0;
        unsigned long last_correlationId = sentObj.correlationId;

        sendQueue.send_n(&sentObj, QueueBase::printSend);
        ready = true;
        TEST_ASSERT_TRUE_MESSAGE(sentObj.correlationId == last_correlationId + 1, "firstObj.correlationId did not get +1");
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

  unsigned long sendInterval = 200;

  xTaskCreatePinnedToCore(SendFirstObjOnInterval::task,
                          "SendFirstObjOnIntervalTask",
                          /*stack*/ 3000,
                          /*params*/ (void *)&sendInterval,
                          /*priority*/ 1,
                          /*handle*/ NULL,
                          /*CORE*/ 0);

  counter = 0;
  firstObj.event_id = 0;

  while (SendFirstObjOnInterval::ready == false)
  {
    vTaskDelay(5);
  }

  elapsedMillis since_last_packet = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    bool gotObj = false, timedout = false;
    do
    {
      vTaskDelay(10);
      gotObj = readQueue.hasValue();
      if (gotObj)
        since_last_packet = 0;
      timedout = since_last_packet > 1000;
    } while (!gotObj && !timedout);

    TEST_ASSERT_FALSE_MESSAGE(timedout, "TIMEDOUT");
    TEST_ASSERT_TRUE_MESSAGE(gotObj, "Didn't getObj");

    counter++;
    Serial.printf("counter: %d\n", counter);

    vTaskDelay(sendInterval);
  }

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void test_queue_hasValue()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another using waitForNewResponse()");

  FirstTestObj sendObj;

  counter = 0;
  sendObj.event_id = 0;

  const int NUM_LOOPS = 5;

  while (counter < NUM_LOOPS)
  {
    ulong sent_event_id = sendObj.event_id;

    sendQueue.send_n(&sendObj);

    TEST_ASSERT_TRUE(sent_event_id == sendObj.event_id - 1);
    DEBUG("PASS: event_id incremented");

    vTaskDelay(5);

    FirstTestObj *res = readQueue.peek();
    TEST_ASSERT_NOT_NULL(res);
    DEBUG("PASS: peek is NOT NULL");

    TEST_ASSERT_TRUE(readQueue.hasValue());
    DEBUG("PASS: queue hasValue()");

    TEST_ASSERT_FALSE(readQueue.hasValue());
    DEBUG("PASS: queue does NOT hasValue() the second time");

    counter++;
    vTaskDelay(500);
  }

  TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
}

void testUtils_waitForNewResponse()
{
  Queue1::Manager<FirstTestObj> sendQueue(xFirstQueueHandle, TICKS_5ms, "sendQueue");
  Queue1::Manager<FirstTestObj> readQueue(xFirstQueueHandle, TICKS_5ms, "readQueue");

  printTestInstructions("Test calls from one task and reads in another using waitForNewResponse()");

  FirstTestObj sendObj;

  counter = 0;
  sendObj.event_id = 0;

  while (counter < 5)
  {
    ulong sent_event_id = sendObj.event_id;
    sendQueue.send_n(&sendObj);

    TEST_ASSERT_TRUE(sent_event_id == sendObj.event_id - 1);
    DEBUG("PASS: event_id incremented");

    vTaskDelay(5);

    FirstTestObj *res = readQueue.peek();

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

void test_queue_hasValue_updates_id()
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
    sendQueue.send_n(&firstObj);
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

void testUtils_waitForNewResp_with_QueueType_from_Notification_task()
{
  Queue1::Manager<SendToBoardNotf> *readNotfQueue = new Queue1::Manager<SendToBoardNotf>(xSendToBoardQueueHandle, TICKS_5ms, "(test)readNotfQueue");

  OrchestratorTask::start();
  OrchestratorTask::sendInterval = PERIOD_500ms;

  printTestInstructions("Test testUtils_waitForNewResp_with_QueueType_from_Notification_task");

  while (OrchestratorTask::thisTask->ready == false)
    vTaskDelay(50);

  OrchestratorTask::thisTask->enable();

  counter = 0;

  while (counter < 5)
  {
    DEBUG("--------------------------");

    uint8_t firstResp = waitForNew(readNotfQueue, 1 * SECONDS, QueueBase::printRead);
    TEST_ASSERT_TRUE_MESSAGE(firstResp == Response::OK, "firstResp was not OK");

    uint8_t secondResp = waitForNew(readNotfQueue, PERIOD_50ms, QueueBase::printRead);
    TEST_ASSERT_TRUE_MESSAGE(secondResp == Response::TIMEOUT, "secondResp was not TIMEOUT");

    counter++;
    vTaskDelay(50);
  }

  TEST_ASSERT_TRUE(counter >= 5);
}
//-----------------------------

uint8_t mockNintendoClassicButtonPress()
{
  Serial.printf("counter: %d\n", counter);
  switch (counter)
  {
  case 0:
    Serial.printf("picking: %s\n", "BUTTON_LEFT");
    return (uint8_t)NintendoController::BUTTON_LEFT;
  case 1:
    Serial.printf("picking: %s\n", "BUTTON_RIGHT");
    return (uint8_t)NintendoController::BUTTON_RIGHT;
  case 2:
    Serial.printf("picking: %s\n", "BUTTON_UP");
    return (uint8_t)NintendoController::BUTTON_UP;
  case 3:
    Serial.printf("picking: %s\n", "BUTTON_START");
    return (uint8_t)NintendoController::BUTTON_START;
  case 4:
    Serial.printf("picking: %s\n", "BUTTON_DOWN");
    return (uint8_t)NintendoController::BUTTON_DOWN;
  case 5:
    Serial.printf("picking: %s\n", "BUTTON_SELECT");
    return (uint8_t)NintendoController::BUTTON_SELECT;
  }
  return (uint8_t)NintendoController::BUTTON_A;
}

// void sendOutNotification_TaskRespondsWithCurrentCorrelationId()
// {
//   mux_I2C = xSemaphoreCreateMutex();

//   mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
//   mutex_I2C.enabled = true;

//   mutex_SPI.create("SPI", /*default*/ TICKS_50ms);
//   mutex_SPI.enabled = true;

//   QwiicButtonTask::qwiicButton.setMockIsPressedCallback([] {
//     return false;
//   });

//   sendNotfQueue = Queue1::Manager<SendToBoardNotf>::create("test)sendNotfQueue");
//   // primaryButtonQueue = QwiicButtonTask::createQueueManager("(test)primaryButtonQueue");
//   // readThrottleQueue = ThrottleTask::createQueueManager("(test)readThrottleQueue");
//   // readPacketStateQueue = BoardCommsTask::createQueueManager("(test)readPacketStateQueue");
//   // readNintendoQueue = NintendoClassicTask::createQueueManager("(test)readNintendoQueue");

//   Queue1::Manager<SendToBoardNotf> *read_NotfQueue = Queue1::Manager<SendToBoardNotf>::create("test)read_NotfQueue");
//   Queue1::Manager<PrimaryButtonState> *primaryButton = Queue1::Manager<PrimaryButtonState>::create("test)primaryButton");

//   // OrchestratorTask::mgr.create(OrchestratorTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
//   // Display::mgr.create(Display::task, /*CORE*/ 0, /*PRIORITY*/ 1);
//   // QwiicButtonTask::mgr.create(QwiicButtonTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
//   // ThrottleTask::mgr.create(ThrottleTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
//   // BoardCommsTask::mgr.create(BoardCommsTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);
//   // NintendoClassicTask::mgr.create(NintendoClassicTask::task, /*CORE*/ 0, /*PRIORITY*/ 1);

//   BaseTaskTest1::start();

//   // NintendoClassicTask::classic.setMockGetButtonEventCallback(mockNintendoClassicButtonPress);

//   // OrchestratorTask::setSendInterval(3 * SECONDS);

//   printTestInstructions("Test testUtils_waitForNewResp_with_QueueType_from_Notification_task");

//   while ( // OrchestratorTask::mgr.ready == false ||
//       //  Display::mgr.ready == false ||
//       //  QwiicButtonTask::mgr.ready == false ||
//       //  ThrottleTask::mgr.ready == false ||
//       //  BoardCommsTask::mgr.ready == false ||
//       //  NintendoClassicTask::mgr.ready == false ||
//       BaseTaskTest1::thisTask->ready == false ||
//       false)
//   {
//     vTaskDelay(50);
//   }

//   BoardCommsTask::boardClient.mockResponseCallback([](ControllerData out) {
//     VescData mockresp;
//     mockresp.id = out.id;
//     return mockresp;
//   });

//   DEBUG("Tasks ready!");

//   OrchestratorTask::mgr.enable(PRINT_THIS);
//   // BoardCommsTask::mgr.enable(PRINT_THIS);

//   SendToBoardNotf notification;
//   notification.correlationId = 10 - 1;

//   static int counter = 0;
//   const int NUM_TEST_LOOPS = 4;

//   while (counter < NUM_TEST_LOOPS)
//   {
//     vTaskDelay(TICKS_50ms);

//     if (sendNotfQueue == nullptr)
//     {
//       DEBUG("sendNotfQueue is NULL!");
//       vTaskDelay(5000);
//     }

//     DEBUG("--------------------------");
//     notification.sent_time = millis();
//     notification.correlationId++;
//     sendNotfQueue->send_n(&notification, QueueBase::printSend);

//     vTaskDelay(TICKS_100ms);

//     uint8_t res = Response::OK;

//     // check NotfQueue
//     res = waitForNew(read_NotfQueue, PERIOD_50ms, QueueBase::printRead);
//     TEST_ASSERT_TRUE_MESSAGE(res == Response::OK, "Did not find event on NotfQueue");
//     TEST_ASSERT_TRUE_MESSAGE(read_NotfQueue->payload.correlationId == notification.correlationId,
//                              "Did not find matching correlationId on NotfQueue");

//     // Primary read
//     res = waitForNew(primaryButton, PERIOD_50ms, QueueBase::printRead);
//     TEST_ASSERT_TRUE_MESSAGE(res == Response::OK, "Did not find event on PrimaryButton queue");
//     TEST_ASSERT_TRUE_MESSAGE(primaryButton->payload.correlationId == notification.correlationId,
//                              "Did not find matching correlationId on PrimaryButton");

//     vTaskDelay(TICKS_100ms);

//     counter++;
//     vTaskDelay(200);
//   }

//   TEST_ASSERT_TRUE(counter == NUM_TEST_LOOPS);

//   BaseTaskTest1::deleteTask(PRINT_THIS);
// }

void setup()
{
  delay(2000);

  Serial.begin(115200);
  delay(100);

  UNITY_BEGIN();

  RUN_TEST(test_calls_and_responds_in_same_task);
  RUN_TEST(test_calls_from_one_task_and_reads_in_another);
  RUN_TEST(test_queue_hasValue);
  RUN_TEST(testUtils_waitForNewResponse);
  RUN_TEST(test_queue_hasValue_updates_id);
  RUN_TEST(testUtils_waitForNewResp_with_QueueType_from_Notification_task);
  // RUN_TEST(sendOutNotification_TaskRespondsWithCurrentCorrelationId);

  UNITY_END();
}

void loop()
{
  vTaskDelete(NULL);
}
