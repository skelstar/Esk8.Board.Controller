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

void onConnect()
{
  Serial.printf("onConnect()\n");
}

void onDisconnect()
{
  Serial.printf("onDisconnect()\n");
}

void onNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic,
              uint8_t *pData,
              size_t length,
              bool isNotify)
{
  Serial.printf("Notify!\n");
}

void hudTask_1(void *pvParameters)
{
  Serial.printf("hudTask_1 running on core %d\n", xPortGetCoreID());

  bleClient.onConnect = onConnect;
  bleClient.onDisconnect = onDisconnect;
  bleClient.onNotify = onNotify;

  while (true)
  {
    // reconnect
    if (!bleClient.isConnected() && sinceLastConnectToHUD > 3000)
    {
      Serial.printf("Trying to connect to server...\n");
      bleServerConnected = bleClient.bleConnectToServer(
          "BLE Client",
          SERVER_UUID,
          SERVICE_UUID,
          CHARACTERISTIC_UUID);
      sinceLastConnectToHUD = 0;
      // send initialising state
      if (bleServerConnected)
      {
        hudMessageQueueManager->send(HUD_EV_FLASH_GREEN);
      }
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

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
