#include "BLEDevice.h"

bool bleServerConnected = false;

// prototypes
bool bleConnectToServer();

template <typename T>
void sendToServer(T data);

elapsedMillis
    sinceHudTaskPulse,
    sinceLastConnectToHUD,
    sinceSentToServer;
/* ---------------------------------------------- */

void hudTask_0(void *pvParameters)
{
  Serial.printf("hudTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    if (sinceHudTaskPulse > 3000)
    {
      sinceHudTaskPulse = 0;
      Serial.printf("HUD pulse\n");
    }

    if (bleServerConnected == false && sinceLastConnectToHUD > 3000)
    {
      Serial.printf("Trying to connect to server...\n");
      bleServerConnected = bleConnectToServer();
      sinceLastConnectToHUD = 0;
    }

    if (bleServerConnected && sinceSentToServer > 3000)
    {
      sinceSentToServer = 0;
      Serial.printf("Trying to send...\n", hudData.id);
      sendToServer(hudData);
      Serial.printf("Sent: %d\n", hudData.id);
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
/* ---------------------------------------------- */

void bleConnected()
{
  bleServerConnected = true;
  Serial.printf("BLE Server connected!");
}
void bleDisconnected()
{
  bleServerConnected = false;
  sinceLastConnectToHUD = 0;
  Serial.printf("BLE Server disconnected!");
}

void bleReceivedNotify()
{
  Serial.printf("Received: id=%d \n", hudData.id);
}

/* ---------------------------------------------- */
static BLEAddress *pServerAddress;
static bool doConnect = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    bleConnected();
  }

  void onDisconnect(BLEClient *pclient)
  {
    bleDisconnected();
  }
};

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  memcpy(&hudData, pData, sizeof(hudData));
  bleReceivedNotify();
}

bool bleConnectToServer()
{
  BLEDevice::init("BLE Client");
  pServerAddress = new BLEAddress(SERVER_UUID); // display-less TTGO
  delay(200);
  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(*pServerAddress);
  delay(500);
  BLERemoteService *pRemoteService = pClient->getService(SERVICE_UUID);
  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic->canNotify())
  {
    Serial.printf("Registering for notify\n");
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
  return true;
}

template <typename T>
void sendToServer(T data)
{
  uint8_t bs[sizeof(hudData)];
  memcpy(&hudData, bs, sizeof(hudData));
  pRemoteCharacteristic->writeValue(bs, sizeof(bs));
}
