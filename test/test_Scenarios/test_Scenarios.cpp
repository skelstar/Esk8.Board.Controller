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
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
}

void tearDown()
{
}

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

void WhenTheBoardsStops_itCreatesNintendoButtonsTask_thenDeletesItWhenMovingAgain()
{
  Queue1::Manager<PacketState> packetStateQueue(xPacketStateQueueHandle, TICKS_5ms, "(test)PacketStateQueue");
  Queue1::Manager<SendToBoardNotf> sendNotfQueue(xSendToBoardQueueHandle, TICKS_5ms, "(test)sendNotfQueue");

  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_0, TASK_PRIORITY_1);
  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, CORE_0, TASK_PRIORITY_1);

  BoardCommsTask::mgr.enable();
  SendToBoardTimerTask::mgr.enable();

  SendToBoardTimerTask::setSendInterval(1000);
  SendToBoardNotf notification;

  BoardCommsTask::boardClient.mockResponseCallback(mockStoppedResponse);

  elapsedMillis since_checked_queue, since_last_notf = 0;

  Serial.printf("------------------------------------\n");

  while (!BoardCommsTask::mgr.ready || !SendToBoardTimerTask::mgr.ready)
  {
    vTaskDelay(5);
  }
  DEBUG("Tasks ready!");

  counter = 0;

  while (counter < NUM_STEPS)
  {
    bool gotResp = false, timedout = false;
    do
    {
      if (sendNotfQueue.hasValue("test loop"))
      {
        gotResp = true;
        since_last_notf = 0;
      }
      timedout = since_last_notf > 2000;
      vTaskDelay(5);
    } while (!gotResp && !timedout);

    TEST_ASSERT_FALSE(timedout);
    TEST_ASSERT_TRUE(gotResp);

    Serial.printf("------------------------------------\n");

    vTaskDelay(50);

    gotResp = false, timedout = false;
    // do
    // {
    //   if (packetStateQueue.hasValue())
    //   {
    //     gotResp = true;
    //     Serial.printf("Board resp, id: %lu moving: %d\n",
    //                   packetStateQueue.payload.event_id,
    //                   packetStateQueue.payload.isMoving());
    //   }
    //   vTaskDelay(5);
    // } while (!gotResp && !timedout);

    TEST_ASSERT_FALSE(timedout);
    TEST_ASSERT_TRUE(gotResp);
    Serial.printf("------------------------------------\n");

    counter++;
    vTaskDelay(100);
  }

  BoardCommsTask::mgr.deleteTask(PRINT_THIS);
  SendToBoardTimerTask::mgr.deleteTask(PRINT_THIS);

  TEST_ASSERT_TRUE(counter == 6);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  RUN_TEST(WhenTheBoardsStops_itCreatesNintendoButtonsTask_thenDeletesItWhenMovingAgain);

  UNITY_END();
}

void loop()
{
}
