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
#include <types/PrimaryButton.h>
#include <types/Throttle.h>
#include <types/PacketState.h>

#include <MockMagThrottle.h>
#include <MockedQwiicButton.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <MockGenericClient.h>

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

//------------------------------------------

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
#include <tasks/core0/SendToBoardTimerTask.h>
#include <tasks/core0/BoardCommsTask.h>

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
//==========================================
void setUp()
{
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
}

//==========================================
void tearDown()
{
}

//------------------------------------------
const int NUM_STEPS = 5;
uint8_t _s_MovingSteps[NUM_STEPS] = {1, 0, 0, 0, 1};

VescData mockMovingResponse(ControllerData out)
{
  VescData mockresp;
  mockresp.id = out.id;
  mockresp.version = VERSION_BOARD_COMPAT;
  mockresp.moving = _s_MovingSteps[counter] == 1;
  return mockresp;
}

//==========================================

void WhenTheNotfIsSentOut_BoardSendsPacketState()
{

  Queue1::Manager<PacketState> *packetStateQueue = new Queue1::Manager<PacketState>(xPacketStateQueueHandle, TICKS_5ms, "(test)PacketStateQueue");
  Queue1::Manager<SendToBoardNotf> *readNotfQueue = new Queue1::Manager<SendToBoardNotf>(xSendToBoardQueueHandle, TICKS_5ms, "(test)readNotfQueue");

  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_0, TASK_PRIORITY_1);
  BoardCommsTask::boardClient.mockResponseCallback(mockMovingResponse);

  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, CORE_0, TASK_PRIORITY_1);

  elapsedMillis since_checked_queue, since_checked_for_notf = 0;

  Serial.printf("------------------------------------\n");

  while (!BoardCommsTask::mgr.ready || !SendToBoardTimerTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  SendToBoardTimerTask::setSendInterval(3000, PRINT_THIS);

  SendToBoardTimerTask::mgr.enable();
  BoardCommsTask::mgr.enable();

  DEBUG("Tasks ready!");

  counter = 0;

  while (counter < NUM_STEPS)
  {
    if (since_checked_for_notf > 100)
    {
      since_checked_for_notf = 0;

      // NOTF
      DEBUG("--------- NOTF > ----------");
      uint8_t resp = waitForNew<SendToBoardNotf>(readNotfQueue, 2 * SECONDS, QueueBase::printRead);
      TEST_ASSERT_TRUE(resp == Response::OK);

      // notification
      resp = waitForNew<SendToBoardNotf>(readNotfQueue, 2 * SECONDS, QueueBase::printRead);
      TEST_ASSERT_TRUE(resp == Response::TIMEOUT);

      // PacketState
      resp = waitForNew<PacketState>(packetStateQueue, 100 * MILLIS_S, QueueBase::printRead);
      TEST_ASSERT_TRUE(resp == Response::OK);

      resp = waitForNew<PacketState>(packetStateQueue, 100, QueueBase::printRead);
      TEST_ASSERT_TRUE(resp == Response::TIMEOUT);
      DEBUG("PASS: found PacketState packet once, not twice");

      counter++;
      vTaskDelay(5);
    }
    vTaskDelay(100);
  }
  BoardCommsTask::mgr.deleteTask(PRINT_THIS);
  SendToBoardTimerTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter == NUM_STEPS);

  vTaskDelete(NULL);
}

//==========================================

template <typename T>
class Calls
{
#define NUM_METHODS_CALLED 20;

public:
  struct Call
  {
    unsigned long time;
    T payload;
    int count;
  };

  Call calls[20];

  Calls()
  {
  }

  void record(uint8_t p_method, T p_payload)
  {
    if (p_method < 20)
    {
      calls[p_method].time = millis();
      calls[p_method].payload = p_payload;
      calls[p_method].count++;

      // DEBUGVAL(((VescData *)p_payload)->moving, ((VescData *)p_payload)->id, ((VescData *)p_payload)->version);
      // DEBUGVAL(((VescData *)calls[p_method].payload)->moving, ((VescData *)calls[p_method].payload)->id, ((VescData *)calls[p_method].payload)->version);
      DEBUGVAL(((VescData)p_payload).moving, ((VescData)p_payload).id, ((VescData)p_payload).version);
      DEBUGVAL(((VescData)calls[p_method].payload).moving, ((VescData)calls[p_method].payload).id, ((VescData)calls[p_method].payload).version);
    }
  }

  Call *getCall(uint8_t method)
  {
    return calls[method].time > 0 ? calls(method) : nullptr;
  }

private:
  int _idx = 0;
};

void WhenMockPacketWithMovingIsSent_SendsPacketWithMovingTrue()
{
  static Calls<VescData> mockCalls;

  Queue1::Manager<PacketState> *packetStateQueue = new Queue1::Manager<PacketState>(xPacketStateQueueHandle, TICKS_5ms, "(test)PacketStateQueue");
  Queue1::Manager<SendToBoardNotf> *sendNotfQueue = new Queue1::Manager<SendToBoardNotf>(xSendToBoardQueueHandle, TICKS_5ms, "(test)sendNotfQueue");

  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_0, TASK_PRIORITY_1);

  // elapsedMillis since_checked_for_notf = 0;

  Serial.printf("------------------------------------\n");

  while (!BoardCommsTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  enum Method
  {
    NONE = 0,
    MockBoardClientResponse,
  };

  BoardCommsTask::mgr.enable();

  BoardCommsTask::boardClient.mockResponseCallback([](ControllerData out) {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = true;
    mockCalls.record((uint8_t)Method::MockBoardClientResponse, mockresp);

    VescData::print(mockresp, "mockresp");

    return mockresp;
  });

  DEBUG("Tasks ready!");

  SendToBoardNotf notification;
  DEBUG("--------- NOTF -> --------");
  // spam the queue a bit first
  sendNotfQueue->send(&notification);
  vTaskDelay(200);
  sendNotfQueue->send(&notification);

  vTaskDelay(500);

  // get packet from the queue
  uint8_t resp = waitForNew<PacketState>(packetStateQueue, PERIOD_500ms, QueueBase::printRead);
  Serial.printf("(test2)packet from boardComms id: %lu moving: %d \n",
                packetStateQueue->payload.packet_id,
                packetStateQueue->payload.moving);

  // assert
  TEST_ASSERT_TRUE_MESSAGE(resp == Response::OK, "there was no new Packet from BoardCommsTask");
  TEST_ASSERT_TRUE_MESSAGE(packetStateQueue->payload.moving, "Board is not showing to be moving");

  TEST_ASSERT_TRUE_MESSAGE(mockCalls.calls[Method::MockBoardClientResponse].time > 0, "mockCalls time was 0");

  TEST_ASSERT_TRUE_MESSAGE(mockCalls.calls[Method::MockBoardClientResponse].payload.moving == 1, "mockCalls moving was not 1");

  // maybe test to see that "stopping" is working

  vTaskDelay(100);

  BoardCommsTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(true);

  vTaskDelete(NULL);
}

void WhenSendingToAQueue_ItStoresTheItemInHistory()
{
  Queue1::Manager<PacketState> *sendToPacketState = new Queue1::Manager<PacketState>(xPacketStateQueueHandle, TICKS_5ms, "(test)sendPacket");

  PacketState packet;

  sendToPacketState->send_r(&packet);
  vTaskDelay(200);

  sendToPacketState->send_r(&packet);
  vTaskDelay(200);

  sendToPacketState->send_r(&packet);
  vTaskDelay(200);

  PacketState::print(sendToPacketState->getFromHistory(0), "history: 0");
  PacketState::print(sendToPacketState->getFromHistory(1), "history: 1");
  PacketState::print(sendToPacketState->getFromHistory(2), "history: 2");

  TEST_ASSERT_EQUAL_MESSAGE(0, sendToPacketState->getFromHistory(0).event_id, "history element 0 not the right one");
  TEST_ASSERT_EQUAL_MESSAGE(1, sendToPacketState->getFromHistory(1).event_id, "history element 1 not the right one");
  TEST_ASSERT_EQUAL_MESSAGE(2, sendToPacketState->getFromHistory(2).event_id, "history element 2 not the right one");

  // maybe test to see that "stopping" is working

  vTaskDelay(100);

  BoardCommsTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(true);

  vTaskDelete(NULL);
}

//==========================================

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  // RUN_TEST(WhenTheNotfIsSentOut_BoardSendsPacketState);
  // RUN_TEST(WhenMockPacketWithMovingIsSent_SendsPacketWithMovingTrue);
  RUN_TEST(WhenSendingToAQueue_ItStoresTheItemInHistory);

  UNITY_END();
}
//==========================================

void loop()
{
}
//==========================================
