#ifndef ARDUINO_H
#include <Arduino.h>
#endif

#include "BLEDevice.h"

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic *pRemoteCharacteristic;

//--------------------------------------------------------------

class BLEClientLib
{
  typedef void (*ConnectCallback)();
  typedef void (*NotifyCallback)(
      BLERemoteCharacteristic *pBLERemoteCharacteristic,
      uint8_t *pData,
      size_t length,
      bool isNotify);

public:
  ConnectCallback onConnect;
  ConnectCallback onDisconnect;
  NotifyCallback onNotify;

  BLEClient *pClient = BLEDevice::createClient();

  BLEClientLib()
  {
    Serial.printf("Initialising BLEClientLib\n");
    _connected = false;
  }

  bool isConnected()
  {
    bool nowConnected = pClient->isConnected();
    if (_connected != nowConnected)
    {
      _connected = nowConnected;
      if (_connected && onConnect != NULL)
      {
        onConnect();
      }
      else if (onDisconnect != NULL)
      {
        onDisconnect();
      }
    }
    return pClient->isConnected();
  }

  // blocking call
  bool bleConnectToServer(char *as, char *serverUUID, char *serviceUUID, char *characteristicUUID)
  {
    BLEDevice::init(as);
    pClient->disconnect();
    pServerAddress = new BLEAddress(serverUUID);
    delay(200);

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(*pServerAddress);
    delay(500);

    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    pRemoteCharacteristic = pRemoteService->getCharacteristic(characteristicUUID);
    if (pRemoteCharacteristic->canNotify() && onNotify != NULL)
    {
      pRemoteCharacteristic->registerForNotify(onNotify);
    }
    return true;
  }

  template <typename T>
  void sendToServer(T data)
  {
    uint8_t bs[sizeof(T)];
    memcpy(bs, &data, sizeof(T));
    pRemoteCharacteristic->writeValue(bs, sizeof(bs));
  }

private:
  bool _connected;

  class MyClientCallback : public BLEClientCallbacks
  {
  public:
    void onConnect(BLEClient *pclient)
    {
      Serial.printf("connected! (MyClientCallback) \n");
    }

    void onDisconnect(BLEClient *pclient)
    {
      Serial.printf("disconnected! (MyClientCallback)");
    }
  };
};
