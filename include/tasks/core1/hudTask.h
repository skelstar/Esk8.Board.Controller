#ifndef elapsedMillis_h
#include <elapsedMillis.h>
#endif

#define SERVER_UUID "D8:A0:1D:5D:AE:9E"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

elapsedMillis
    sinceHudReadFromQueue,
    sinceLastConnectToHUD,
    sinceCheckedActionQueue,
    sinceSentToServer;
/* ---------------------------------------------- */

bool hasConnectedToHud = false;

/* ---------------------------------------------- */

namespace HUD
{
  void task(void *pvParameters)
  {
    Serial.printf("hudTask_1 running on CORE_%d\n", xPortGetCoreID());

    hudActionQueue->setSentEventCallback([](uint8_t ev) {
      if (PRINT_HUD_ACTION_QUEUE_SEND)
        Serial.printf(HUD_ACTION_QUEUE_SENT_FORMAT, HUDAction::getName(ev));
    });
    hudActionQueue->setReadEventCallback([](uint8_t ev) {
      if (PRINT_HUD_ACTION_QUEUE_READ)
        Serial.printf(HUD_ACTION_QUEUE_READ_FORMAT, HUDAction::getName(ev));
    });

    while (true)
    {
      if (sinceCheckedActionQueue > 100)
      {
        sinceCheckedActionQueue = 0;
        HUDAction::Event action = hudActionQueue->read<HUDAction::Event>();
        if (action != HUDAction::NONE)
        {
        }
      }
      // read from queue
      if (sinceHudReadFromQueue > 500)
      {
        sinceHudReadFromQueue = 0;
        if (hudQueue->messageAvailable())
        {
          using namespace HUDCommand1;

          uint8_t task = hudQueue->read<HUDTask::Message>();

          if (hudClient.connected())
          {
            uint16_t command = 0;
            switch (task)
            {
            case HUDTask::NONE:
              command = 1 << HEARTBEAT;
              break;
            case HUDTask::BOARD_DISCONNECTED:
              command = 1 << PULSE | 1 << RED;
              break;
            case HUDTask::BOARD_CONNECTED:
              command = 1 << HEARTBEAT;
              break;
            case HUDTask::WARNING_ACK:
              command = 1 << GREEN | 1 << FLASH;
              break;
            case HUDTask::CONTROLLER_RESET:
              command = 1 << RED | 1 << FLASH | 1 << SLOW;
              break;
            case HUDTask::BOARD_MOVING:
              command = 1 << GREEN | 1 << FLASH;
              break;
            case HUDTask::BOARD_STOPPED:
              command = 1 << RED | 1 << FLASH | 1 << SLOW;
              break;
            case HUDTask::HEARTBEAT:
              command = 1 << BLUE | 1 << TWO_FLASHES;
              break;
            case HUDTask::ACKNOWLEDGE:
              command = 1 << GREEN | 1 << TWO_FLASHES;
              break;
            case HUDTask::CYCLE_BRIGHTNESS:
              command = 1 << CYCLE_BRIGHTNESS;
              break;
            case HUDTask::GO_TO_IDLE:
              command = 1 << HEARTBEAT;
              break;
            default:
              break;
            }

            sendCommandToHud(command);
          }
        }
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