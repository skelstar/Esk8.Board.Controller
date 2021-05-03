#pragma once

// #include <tasks/root.h>
// #include <tasks/queues/types/root.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

// #define ASSERT(condition, message)                                                        \
//   do                                                                                      \
//   {                                                                                       \
//     if (!(condition))                                                                     \
//     {                                                                                     \
//       Serial.printf("Assertion: %s | file: %s line: %s \n", message, __FILE__, __LINE__); \
//     }                                                                                     \
//   } while (false);

namespace Test
{
  void waitForTasksReady()
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

    Serial.print("[TEST] all tasks ready\n");
  }

  void enableAllTasks(bool print = false)
  {
    boardCommsTask.enable(print);
    displayTask.enable(print);
    nintendoClassTask.enable(print);
    qwiicButtonTask.enable(print);
    remoteTask.enable(print);
    throttleTask.enable(print);

    Serial.print("[TEST] all tasks enabled\n");
  }

  void printLine(const char *start, int dashes = 10)
  {
    Serial.print(start);
    int i = 0;
    while (i++ < dashes)
      Serial.print("-");
    Serial.println();
  }

  void printTestInstructions(const char *instructions)
  {
    vTaskDelay(TICKS_500ms);
    const char *st = "*** ";
    printLine(st, strlen(instructions) + 7);
    Serial.printf("*** INSTR: %s\n", instructions);
    printLine(st, strlen(instructions) + 7);
    vTaskDelay(TICKS_500ms);
  }

  void printTestTitle(const char *name)
  {
    vTaskDelay(TICKS_500ms);
    const char *st = "*** ";
    printLine(st, strlen(name) + 7);
    Serial.printf("*** TEST: %s\n", name);
    printLine(st, strlen(name) + 7);
    vTaskDelay(TICKS_500ms);
  }

  void printPASS(const char *message)
  {
    Serial.printf("[PASS @%lums] %s\n", millis(), message);
  }

  void setupAllTheTasks(const char *file = "File not provided")
  {
    DEBUG("----------------------------");
    Serial.printf("    %s \n", file);
    DEBUG("----------------------------");

    xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
    xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
    xPacketStateQueueHandle = xQueueCreate(1, sizeof(BoardState *));
    xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
    xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));

    // configure queues
    displayEventQueue = createQueueManager<DisplayEvent>("(test)displayEventQueue");
    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(test)primaryButtonQueue");
    packetStateQueue = createQueueManager<BoardState>("(test)packetStateQueue");
    nintendoQueue = createQueueManager<NintendoButtonEvent>("(test)nintendoQueue");
    throttleQueue = createQueueManager<ThrottleState>("(test)throttleQueue");

    boardCommsTask.start(BoardComms::task1);
    displayTask.start(Display::task1);
    nintendoClassTask.start(nsNintendoClassicTask::task1);
    qwiicButtonTask.start(nsQwiicButtonTask::task1);
    remoteTask.start(nsRemoteTask::task1);
    throttleTask.start(nsThrottleTask::task1);

    Serial.print("[TEST] all tasks started\n");
  }

  void tearDownAllTheTasks()
  {
    boardCommsTask.deleteTask(PRINT_THIS);
    displayTask.deleteTask(PRINT_THIS);
    nintendoClassTask.deleteTask(PRINT_THIS);
    qwiicButtonTask.deleteTask(PRINT_THIS);
    remoteTask.deleteTask(PRINT_THIS);
    throttleTask.deleteTask(PRINT_THIS);
  }

  VescData mockBoardStoppedResponse(ControllerData out)
  {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = false;
    // Serial.printf("[%lu] mockMovingResponse called, moving: %d\n", millis(), mockresp.moving);
    return mockresp;
  }
} // end namespace