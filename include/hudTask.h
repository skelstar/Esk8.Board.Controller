#include "BLEDevice.h"

#ifndef elapsedMillis_h
#include <elapsedMillis.h>
#endif

#define SERVER_UUID "D8:A0:1D:5D:AE:9E"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool bleServerConnected = false;

elapsedMillis
    sinceHudTaskPulse,
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

void hudTask_0(void *pvParameters)
{
  Serial.printf("hudTask_0 running on core %d\n", xPortGetCoreID());

  bleClient.onConnect = onConnect;
  bleClient.onDisconnect = onDisconnect;
  bleClient.onNotify = onNotify;

  while (true)
  {
    if (sinceHudTaskPulse > 3000)
    {
      sinceHudTaskPulse = 0;
      Serial.printf("HUD pulse\n");
    }

    if (!bleClient.isConnected() && sinceLastConnectToHUD > 3000)
    {
      Serial.printf("Trying to connect to server...\n");
      bleServerConnected = bleClient.bleConnectToServer(
          "BLE Client",
          SERVER_UUID,
          SERVICE_UUID,
          CHARACTERISTIC_UUID);
      sinceLastConnectToHUD = 0;
    }

    if (bleServerConnected && sinceSentToServer > 3000)
    {
      sinceSentToServer = 0;
      bleClient.sendToServer(hudData);
      Serial.printf("Sent: %d\n", hudData.id);
      hudData.id++;
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

// /* ---------------------------------------------- */
// static BLEAddress *pServerAddress;
// static BLERemoteCharacteristic *pRemoteCharacteristic;

// class MyClientCallback : public BLEClientCallbacks
// {
//   void onConnect(BLEClient *pclient)
//   {
//     bleServerConnected = true;
//     sinceLastConnectToHUD = 0;
//     Serial.printf("connected! \n");
//   }

//   void onDisconnect(BLEClient *pclient)
//   {
//     sinceLastConnectToHUD = 0;
//     bleServerConnected = false;
//     Serial.printf("disconnected!");
//   }
// };

// void notifyCallback(
//     BLERemoteCharacteristic *pBLERemoteCharacteristic,
//     uint8_t *pData,
//     size_t length,
//     bool isNotify)
// {
//   memcpy(&hudData, pData, sizeof(hudData));
//   Serial.printf("Received: id=%d \n", hudData.id);
// }

// bool bleConnectToServer()
// {
//   BLEDevice::init("BLE Client");
//   pServerAddress = new BLEAddress(SERVER_UUID);
//   delay(200);
//   BLEClient *pClient = BLEDevice::createClient();
//   pClient->setClientCallbacks(new MyClientCallback());
//   pClient->connect(*pServerAddress);
//   delay(500);
//   BLERemoteService *pRemoteService = pClient->getService(SERVICE_UUID);
//   pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
//   if (pRemoteCharacteristic->canNotify())
//   {
//     Serial.printf("Registering for notify\n");
//     pRemoteCharacteristic->registerForNotify(notifyCallback);
//   }
//   return true;
// }

// template <typename T>
// void sendToServer(T data)
// {
//   uint8_t bs[sizeof(T)];
//   memcpy(bs, &data, sizeof(T));
//   pRemoteCharacteristic->writeValue(bs, sizeof(bs));
// }
