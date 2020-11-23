#include "BLEDevice.h"

#ifndef elapsedMillis_h
#include <elapsedMillis.h>
#endif

#define SERVER_UUID "D8:A0:1D:5D:AE:9E"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool bleServerConnected = false;

elapsedMillis
    sinceHudReadFromQueue,
    sinceLastConnectToHUD,
    sinceSentToServer;
/* ---------------------------------------------- */
#include <BLEClientLib.h>

BLEClientLib bleClient;

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
  hudMessageQueueManager->send(HUD_EV_FLASH_GREEN);
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

void onNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic,
              uint8_t *pData,
              size_t length,
              bool isNotify)
{
  HudActionEvent ev;
  memcpy(&ev, pData, length);
  hudActionQueueManager->send(ev);
  Serial.printf("Notify: %s\n", hudActionEventNames[(int)ev]);
}

void hudTask_1(void *pvParameters)
{
  Serial.printf("hudTask_1 running on CORE_%d\n", xPortGetCoreID());

  bleClient.onConnect = onConnect;
  bleClient.onDisconnect = onDisconnect;
  bleClient.onNotify = onNotify;

  taskState = TASK_RUNNING;

  while (taskState == TASK_RUNNING)
  {
    // connect if not connected before
    if (false == bleClient.isConnected())
    {
      Serial.printf("Trying to connect to server...\n");
      bleServerConnected = bleClient.bleConnectToServer(
          "BLE Client",
          SERVER_UUID,
          SERVICE_UUID,
          CHARACTERISTIC_UUID);
    }

    // read from queue
    if (bleClient.isConnected() && sinceHudReadFromQueue > 500)
    {
      sinceHudReadFromQueue = 0;
      if (hudMessageQueueManager->messageAvailable())
      {
        HUDEvent message = (HUDEvent)hudMessageQueueManager->read();
        hudData.state = message;
        bleClient.sendToServer(hudData);
        Serial.printf("Sent event:%s\n", eventToString(hudData.state));
        hudData.id++;
      }
    }

    if (hudActionQueueManager->messageAvailable())
    {
      HudActionEvent action = (HudActionEvent)hudActionQueueManager->read();
      Serial.printf("queue rx: %s\n", hudActionEventNames[(int)action]);
    }

    vTaskDelay(10);
  }

  Serial.printf("sending hudTask_1 restart message\n");
  taskQueueManager->send(1);

  vTaskDelay(100);
  Serial.printf("should restart before this prints\n");

  vTaskDelete(NULL); // deletes the current task
}

void createHudTask()
{
  xTaskCreatePinnedToCore(
      hudTask_1,
      "hudTask_1",
      5000,
      NULL,
      TASK_PRIORITY_1,
      &hudTaskHandle,
      CORE_1);
}
