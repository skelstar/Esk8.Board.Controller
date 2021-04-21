// #include <Arduino.h>
// #include <unity.h>

// #define DEBUG_SERIAL 1

// #ifdef DEBUG_SERIAL
// #define DEBUG_OUT Serial
// #endif
// #define PRINTSTREAM_FALLBACK
// #include "Debug.hpp"

// #define PRINT_MUTEX_TAKE_FAIL 1
// static int counter = 0;

// // RTOS ENTITES-------------------

// QueueHandle_t xFirstQueueHandle;
// QueueHandle_t xOtherTestQueueHandle;

// #include <tasks/queues/queues.h>

// SemaphoreHandle_t mux_I2C;
// SemaphoreHandle_t mux_SPI;

// #include <types.h>
// #include <rtosManager.h>
// #include <QueueManager1.h>
// #include <elapsedMillis.h>
// #include <RTOSTaskManager.h>
// #include <BoardClass.h>
// #include <testUtils.h>
// #include <Wire.h>

// MyMutex mutex_I2C;
// MyMutex mutex_SPI;

// #include <tasks/queues/types/QueueBase.h>
// #include <tasks/queues/types/PacketState.h>
// #include <tasks/queues/types/NintendoButtonEvent.h>
// #include <tasks/queues/types/PrimaryButton.h>
// #include <tasks/queues/types/Throttle.h>

// #define RADIO_OBJECTS
// // NRF24L01Lib nrf24;

// // #include <MockQwiicButton.h>

// // #include <displayState.h>

// // TASKS ------------------------

// // #include <tasks/core0/DisplayTask.h>
// // #include <tasks/core0/ThrottleTask.h>
// // #include <tasks/core0/BoardCommsTask.h>
// // #include <tasks/core0/NintendoClassicTask.h>
// // #include <tasks/core0/remoteTask.h>

// #include <tasks/core0/QwiicTaskBase.h>
// #include <tasks/core0/BoardCommsTask.h>

// //----------------------------------

// void printTestTitle(const char *name)
// {
//   Serial.printf("-------------------------------------------\n");
//   Serial.printf("  TEST: %s\n", name);
//   Serial.printf("-------------------------------------------\n");
// }

// void printTestInstructions(const char *instructions)
// {
//   Serial.printf("*** INSTR: %s\n", instructions);
// }

// void setUp()
// {
//   DEBUG("----------------------------");
//   Serial.printf("    %s \n", __FILE__);
//   DEBUG("----------------------------");

//   xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
//   xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
//   xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
//   xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
//   xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));
// }

// void tearDown()
// {
// }

// //-----------------------------------------------
// void usesTaskSchedulerAndQwiicButton_withTaskBasesAnRealButton_sendsPacketsAndRespondsOK()
// {
//   Wire.begin();
//   // start tasks
//   TaskScheduler::start();
//   TaskScheduler::sendInterval = PERIOD_500ms;
//   TaskScheduler::printSendToSchedule = false;

//   QwiicTaskBase::start();
//   QwiicTaskBase::printReplyToSchedule = false;

//   // configure queues
//   Queue1::Manager<PrimaryButtonState> *primaryButtonStateQueue = Queue1::Manager<PrimaryButtonState>::create("(test)primaruButtonStateQueue");

//   // mocks

//   // wait
//   while (TaskScheduler::thisTask->ready == false ||
//          QwiicTaskBase::thisTask->ready == false ||
//          false)
//   {
//     vTaskDelay(10);
//   }

//   DEBUG("Tasks ready");

//   vTaskDelay(PERIOD_100ms);

//   TaskScheduler::thisTask->enable(PRINT_THIS);
//   QwiicTaskBase::thisTask->enable(PRINT_THIS);

//   counter = 0;

//   const int NUM_LOOPS = 5;
//   elapsedMillis since_started_testing = 0;

//   while (since_started_testing < 8 * SECONDS)
//   {
//     // confirm schedule packet on queue
//     uint8_t response = waitForNew(scheduleQueue, PERIOD_1s, nullptr, PRINT_TIMEOUT);
//     TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find schedule packet on the schedule queue");

//     // check for response from Primary Button (Qwiic)
//     response = waitForNew(primaryButtonStateQueue, PERIOD_50ms, nullptr, PRINT_TIMEOUT);
//     TEST_ASSERT_TRUE_MESSAGE(response == Response::OK, "Didn't find primaryButton on the queue");
//     TEST_ASSERT_EQUAL_MESSAGE(scheduleQueue->payload.event_id,
//                               primaryButtonStateQueue->payload.event_id,
//                               "PrimaryButtonState event_id does not match");

//     const char *pressed = primaryButtonStateQueue->payload.pressed == true ? "YES" : " - ";
//     Serial.printf("Primary Button pressed: %s\n", pressed);
//     counter++;

//     vTaskDelay(10);
//   }

//   TaskScheduler::thisTask->deleteTask(PRINT_THIS);
//   QwiicTaskBase::thisTask->deleteTask(PRINT_THIS);

//   TEST_ASSERT_TRUE(counter >= NUM_LOOPS);
// }
// //-----------------------------------------------

// //===================================================================

// void setup()
// {
//   delay(2000);

//   Serial.begin(115200);
//   delay(100);

//   UNITY_BEGIN();

//   RUN_TEST(usesTaskSchedulerAndQwiicButton_withTaskBasesAnRealButton_sendsPacketsAndRespondsOK);

//   UNITY_END();
// }

// void loop()
// {
//   vTaskDelete(NULL);
// }
