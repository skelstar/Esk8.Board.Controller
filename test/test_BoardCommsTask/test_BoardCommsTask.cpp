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

VescData mockStoppedResponse(ControllerData out)
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
  BoardCommsTask::boardClient.mockResponseCallback(mockStoppedResponse);

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

      bool gotResp = false, timedout = false;

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

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  RUN_TEST(WhenTheNotfIsSentOut_BoardSendsPacketState);

  UNITY_END();
}
//==========================================

void loop()
{
}
//==========================================
