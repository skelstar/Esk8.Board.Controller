#ifndef elapsedMillis_h
#include <elapsedMillis.h>
#endif

#define SERVER_UUID "D8:A0:1D:5D:AE:9E"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

elapsedMillis
    sinceCheckedTaskForHUDQueue,
    sinceLastConnectToHUD,
    sinceCheckedActionQueue,
    sinceSentToServer;
/* ---------------------------------------------- */

bool hasConnectedToHud = false;

HUD::Instruction mapActionToInstruction(uint16_t action);
HUD::Instruction mapTaskToInstruction(uint16_t task);

/* ---------------------------------------------- */

void hudActionQueueSentCb(uint16_t ev)
{
  if (PRINT_HUD_ACTION_QUEUE_SEND)
    Serial.printf(PRINT_QUEUE_SEND_FORMAT, "HUD_ACTION", HUDAction::getName(ev));
}
void hudActionQueueReadCb(uint16_t ev)
{
  if (PRINT_HUD_ACTION_QUEUE_READ)
    Serial.printf(PRINT_QUEUE_READ_FORMAT, "HUD_ACTION", HUDAction::getName(ev));
}

// messages from HUD
void readActionsFromHUDQueue()
{
  using namespace HUD;
  uint16_t action = hudActionQueue->read<HUDAction::Event>();
  if (action >= HUDAction::Length)
  {
    if (DEBUG_BUILD)
      Serial.printf("WARNING: action from HUD queue is OUT OF RANGE (%d)\n", action);
    return;
  }

  if (action == HUDAction::NONE)
    return;

  HUD::Instruction instruction = mapActionToInstruction(action);
  if (FEATURE_SEND_TO_HUD)
    sendInstructionToHud(instruction);
}

HUD::Instruction mapActionToInstruction(uint16_t action)
{
  using namespace HUDAction;
  switch (action)
  {
  case ONE_CLICK:
    return HUD::CYCLE_BRIGHTNESS;
  }
  return 0;
}

HUD::Instruction mapTaskToInstruction(uint16_t task)
{
  using namespace HUD;
  switch (task)
  {
  case HUDTask::BOARD_DISCONNECTED:
    return Instruction(PULSE | RED);
  case HUDTask::BOARD_CONNECTED:
    return Instruction(TWO_FLASHES | GREEN | FAST);
  case HUDTask::WARNING_ACK:
    return Instruction(GREEN | FLASH);
  case HUDTask::CONTROLLER_RESET:
    return Instruction(RED | FLASH | SLOW);
  case HUDTask::BOARD_MOVING:
    return Instruction(GREEN | FLASH);
  case HUDTask::BOARD_STOPPED:
    return Instruction(RED | FLASH | SLOW);
  case HUDTask::HEARTBEAT:
    return Instruction(BLUE | TWO_FLASHES);
  case HUDTask::ACKNOWLEDGE:
    return Instruction(GREEN | TWO_FLASHES);
  case HUDTask::CYCLE_BRIGHTNESS:
    return Instruction(CYCLE_BRIGHTNESS);
  case HUDTask::GO_TO_IDLE:
    return Instruction(HEARTBEAT);
  }
  return Instruction(0);
}

void readTasksFromHUDQueue()
{
  if (!FEATURE_SEND_TO_HUD)
    return;

  using namespace HUD;

  uint16_t task = hudTasksQueue->read<HUDTask::Message>();

  if (hudClient.connected() == false)
  {
    // Serial.printf("hud not connected\n");
    return;
  }

  Instruction instruction = mapTaskToInstruction(task);
  if (instruction.get() != 0)
    if (FEATURE_SEND_TO_HUD)
      sendInstructionToHud(instruction);
}
/* ---------------------------------------------- */

namespace HUD
{
  bool taskReady = false;

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "hudTask_1", xPortGetCoreID());

    hudActionQueue->setSentEventCallback(hudActionQueueSentCb);
    hudActionQueue->setReadEventCallback(hudActionQueueReadCb);

    taskReady = true;

    while (!Comms::taskReady)
    {
      vTaskDelay(5);
    }

    while (true)
    {
      if (sinceCheckedActionQueue > 100)
      {
        sinceCheckedActionQueue = 0;
        readActionsFromHUDQueue();
      }
      // read from queue
      if (sinceCheckedTaskForHUDQueue > 300)
      {
        sinceCheckedTaskForHUDQueue = 0;
        readTasksFromHUDQueue();
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL); // deletes the current task
  }                    // namespace HUD

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "hudTask_1",
        5000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace HUD