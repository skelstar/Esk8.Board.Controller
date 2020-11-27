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

enum TaskState
{
  TASK_RUNNING,
  TASK_RESETTING
};
TaskState taskState = TASK_RUNNING;

void onConnect()
{
  Serial.printf("---------------------------------\n");
  Serial.printf("           onConnect()\n");
  Serial.printf("---------------------------------\n");
  stats.hudConnected = true;
  stats.update = true;
  hudMessageQueue->send(HUD_CMD_FLASH_GREEN);
  hasConnectedToHud = true;
}

void onDisconnect()
{
  Serial.printf("---------------------------------\n");
  Serial.printf("           onDisconnect()\n");
  Serial.printf("---------------------------------\n");
  stats.hudConnected = false;
  stats.update = true;
  taskState = TASK_RESETTING;
}

void onNotify()
{
  // HudActionEvent ev;
  // memcpy(&ev, pData, length);
  // hudActionQueueManager->send(ev);
  // Serial.printf("Notify: %s\n", hudActionEventNames[(int)ev]);
}

void hudTask_1(void *pvParameters)
{
  Serial.printf("hudTask_1 running on CORE_%d\n", xPortGetCoreID());

  taskState = TASK_RUNNING;

  while (taskState == TASK_RUNNING)
  {

    // read from queue
    if (stats.hudConnected && sinceHudReadFromQueue > 500)
    {
      sinceHudReadFromQueue = 0;
      if (hudMessageQueue->messageAvailable())
      {
        HUDCommand message = (HUDCommand)hudMessageQueue->read();
        sendPacketToHud(message, true);
      }
    }

    if (hudActionQueueManager->messageAvailable())
    {
      HudActionEvent action = (HudActionEvent)hudActionQueueManager->read();
      Serial.printf("queue rx: %s\n", hudActionEventNames[(int)action]);
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
      &hudTaskHandle,
      core);
}
