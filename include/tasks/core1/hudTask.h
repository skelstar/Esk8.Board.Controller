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

HUD::Command mapActionToCommand(uint16_t action);
HUD::Command mapTaskToCommand(uint16_t task);

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
    Serial.printf("WARNING: action from HUD queue is OUT OF RANGE (%d)\n", action);
    return;
  }

  if (action == HUDAction::NONE)
    return;

  HUD::Command command = mapActionToCommand(action);
  Serial.printf("Sending to HUD %s\n", command.getCommand());
  sendCommandToHud(command);
}

HUD::Command mapActionToCommand(uint16_t action)
{
  using namespace HUDAction;
  switch (action)
  {
  case ONE_CLICK:
    return HUD::CYCLE_BRIGHTNESS;
    // case HEARTBEAT:
    // case TWO_CLICK:
    // case THREE_CLICK:
    //   break;
  }
  return 0;
}

HUD::Command mapTaskToCommand(uint16_t task)
{
  using namespace HUD;
  switch (task)
  {
  case HUDTask::BOARD_DISCONNECTED:
    return Command(PULSE | RED);
  case HUDTask::BOARD_CONNECTED:
    return Command(HEARTBEAT);
  case HUDTask::WARNING_ACK:
    return Command(GREEN | FLASH);
  case HUDTask::CONTROLLER_RESET:
    return Command(RED | FLASH | SLOW);
  case HUDTask::BOARD_MOVING:
    return Command(GREEN | FLASH);
  case HUDTask::BOARD_STOPPED:
    return Command(RED | FLASH | SLOW);
  case HUDTask::HEARTBEAT:
    return Command(BLUE | TWO_FLASHES);
  case HUDTask::ACKNOWLEDGE:
    return Command(GREEN | TWO_FLASHES);
  case HUDTask::CYCLE_BRIGHTNESS:
    return Command(CYCLE_BRIGHTNESS);
  case HUDTask::GO_TO_IDLE:
    return Command(HEARTBEAT);
  }
  return Command(0);
}

void readTasksForHUDQueue()
{
  using namespace HUD;
  uint16_t task = hudTasksQueue->read<HUDTask::Message>();

  if (hudClient.connected() == false)
    return;

  if (hudClient.connected())
  {
    Command command = mapTaskToCommand(task);
    if (command.get() != 0)
      sendCommandToHud(command);
  }
}
/* ---------------------------------------------- */

namespace HUD
{
  void task(void *pvParameters)
  {
    Serial.printf("hudTask_1 running on CORE_%d\n", xPortGetCoreID());

    hudActionQueue->setSentEventCallback(hudActionQueueSentCb);
    hudActionQueue->setReadEventCallback(hudActionQueueReadCb);

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
        readTasksForHUDQueue();
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