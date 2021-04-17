#include <Wire.h>
#include <Arduino.h>

#define __SparkFun_Qwiic_Button_H__

#define DEFAULT_ADDRESS 0x6F //default I2C address of the button

typedef bool (*MockIsPressedCallback)();

class QwiicButton
{
private:
  MockIsPressedCallback _mockedIsPressedCallback = nullptr;

public:
  bool begin(uint8_t address = DEFAULT_ADDRESS, TwoWire &wirePort = Wire)
  {
    if (_mockedIsPressedCallback == nullptr)
      DEBUG("WARNING: _mockedIsPressedCallback has not been initialised!");
    return true;
  }

  bool isPressed()
  {
    return _mockedIsPressedCallback();
  }

  bool isConnected()
  {
    return true;
  }

  // mocked routines

  void setMockIsPressedCallback(MockIsPressedCallback cb)
  {
    _mockedIsPressedCallback = cb;
  }
};