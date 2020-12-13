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
          HUDTask::Message message = hudQueue->read<HUDTask::Message>();

          if (hudClient.connected())
          {
            sendMessageToHud(message);
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