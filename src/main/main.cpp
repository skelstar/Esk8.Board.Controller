#include <Arduino.h>
#ifndef UNIT_TEST

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <tasks/queues/queues.h>
#include <tasks/queues/types/root.h>

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

#include <types.h>
#include <rtosManager.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <Wire.h>
#include <SparkFun_Qwiic_Button.h>
#include <MagThumbwheel.h>

#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>
RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);
NRF24L01Lib nrf24;

#include <rom/rtc.h> // for reset reason
#include <shared-utils.h>
#include <constants.h>

// TASKS ------------------------

#include <tasks/root.h>

// LOCAL QUEUE MANAGERS -----------------------

Queue1::Manager<BoardState> *packetStateQueue = nullptr;

//------------------------------------------------------------------

#include <utils.h>

void createQueues();
void createLocalQueueManagers();
void startTasks();
void configureTasks();
void waitForTasks();
void enableTasks(bool print = false);

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("------------------------ BOOT ------------------------\n");

  Wire.begin();

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

#ifdef RELEASE_BUILD
  print_build_status(chipId);
#endif
  vTaskDelay(100);

  createQueues();

  createLocalQueueManagers();

  configureTasks();

  startTasks();

  waitForTasks();

  enableTasks();
}
//---------------------------------------------------------------

elapsedMillis since_checked_queues = 0;
BoardState board;

void loop()
{
  if (since_checked_queues > PERIOD_200ms)
  {
    since_checked_queues = 0;

    // changed?
    if (packetStateQueue->hasValue() &&
        board.moving != packetStateQueue->payload.moving)
    {
      // moving
      if (packetStateQueue->payload.moving)
      {
        nintendoClassTask.enabled = false;
        remoteTask.enabled = false;
      }
      // stopped
      else
      {
        nintendoClassTask.enabled = true;
        remoteTask.enabled = true;
      }

      board.moving = packetStateQueue->payload.moving;
    }
  }

  vTaskDelay(TICKS_10ms);
}

//------------------------------------------------------------------

void createQueues()
{
  xBatteryInfo = xQueueCreate(1, sizeof(BatteryInfo *));
  xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  xPacketStateQueueHandle = xQueueCreate(1, sizeof(BoardState *));
  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));
}

void createLocalQueueManagers()
{
  packetStateQueue = createQueueManager<BoardState>("(main) packetStateQueue");
}

void configureTasks()
{
  boardCommsTask.doWorkInterval = PERIOD_50ms;
  boardCommsTask.printRadioDetails = PRINT_NRF24L01_DETAILS;
  // boardCommsTask.printSentPacketToBoard = true;
  // boardCommsTask.printRxPacket = true;

  nintendoClassTask.doWorkInterval = PERIOD_100ms;

  qwiicButtonTask.doWorkInterval = PERIOD_100ms;

  throttleTask.doWorkInterval = PERIOD_200ms;
  throttleTask.printWarnings = true;
  throttleTask.printThrottle = true;
  throttleTask.thumbwheel.setSweepAngle(30.0);
  throttleTask.thumbwheel.setDeadzone(5.0);

  displayTask.doWorkInterval = PERIOD_100ms;
  displayTask.p_printState = PRINT_DISP_STATE;
  displayTask.p_printTrigger = PRINT_DISP_STATE_EVENT;

  remoteTask.doWorkInterval = SECONDS * 5;
  remoteTask.printSendToQueue = true;
}

void startTasks()
{
  boardCommsTask.start(BoardComms::task1);
  displayTask.start(Display::task1);
  nintendoClassTask.start(nsNintendoClassicTask::task1);
  qwiicButtonTask.start(nsQwiicButtonTask::task1);
  remoteTask.start(nsRemoteTask::task1);
  throttleTask.start(nsThrottleTask::task1);
}

void waitForTasks()
{
  while (
      boardCommsTask.ready == false ||
      displayTask.ready == false ||
      nintendoClassTask.ready == false ||
      qwiicButtonTask.ready == false ||
      remoteTask.ready == false ||
      throttleTask.ready == false ||
      false)
    vTaskDelay(PERIOD_10ms);
}

void enableTasks(bool print)
{
  boardCommsTask.enable(print);
  displayTask.enable(print);
  nintendoClassTask.enable(print);
  qwiicButtonTask.enable(print);
  remoteTask.enable(print);
  throttleTask.enable(print);
}

#endif // UNIT_TEST