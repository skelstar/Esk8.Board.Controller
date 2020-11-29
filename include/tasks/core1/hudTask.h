#ifndef elapsedMillis_h
#include <elapsedMillis.h>
#endif

#define SERVER_UUID "D8:A0:1D:5D:AE:9E"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

elapsedMillis
    sinceHudReadFromQueue,
    sinceLastConnectToHUD,
    sinceSentToServer;
/* ---------------------------------------------- */

bool hasConnectedToHud = false;

/* ---------------------------------------------- */

void hudTask_1(void *pvParameters)
{
  Serial.printf("hudTask_1 running on CORE_%d\n", xPortGetCoreID());

  while (true)

  {
    if (hudActionQueue->messageAvailable())
    {
      HUDAction::Event action = hudActionQueue->read<HUDAction::Event>();
      Serial.printf("queue rx: %s\n", HUDAction::names[(int)action]);
    }
    // read from queue
    if (sinceHudReadFromQueue > 500)
    {
      sinceHudReadFromQueue = 0;
      if (hudCommandQueue->messageAvailable())
      {
        HUDTask::Message message = hudCommandQueue->read<HUDTask::Message>();
        HUDCommand::Mode mode = HUDCommand::MODE_NONE;
        HUDCommand::Colour colour = HUDCommand::BLACK;
        switch (message)
        {
        case HUDTask::NONE:
          mode = HUDCommand::Mode::MODE_NONE;
          colour = HUDCommand::Colour::BLACK;
          break;
        case HUDTask::BOARD_DISCONNECTED:
          mode = HUDCommand::Mode::PULSE;
          colour = HUDCommand::Colour::GREEN;
          break;
        case HUDTask::WARNING_ACK:
          mode = HUDCommand::Mode::MODE_NONE;
          colour = HUDCommand::Colour::BLACK;
          break;
        case HUDTask::CONTROLLER_RESET:
          mode = HUDCommand::Mode::SPIN;
          colour = HUDCommand::Colour::RED;
          break;
        default:
          break;
        }

        if (hud.connected)
        {
          bool ok = sendCommandToHud(mode, colour, true);
          if (!ok)
          {
            hud.connected = false;
          }
        }
      }
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL); // deletes the current task
}

void createHudTask(uint8_t core, uint8_t priority)
{
  xTaskCreatePinnedToCore(
      hudTask_1,
      "hudTask_1",
      5000,
      NULL,
      priority,
      NULL,
      core);
}