
class SettingHandlerBase
{
public:
  virtual void display() = 0;
  virtual void changeValue() = 0;
};

class PushToStart : public SettingHandlerBase
{
public:
  PushToStart(char *text)
  {
    _text = text;
    _currVal = featureService.get<bool>(FeatureType::PUSH_TO_START);
  }

  void display()
  {
    const char *value = _currVal == true ? "ON" : "OFF";
    screenPropValue<bool>(_text, value);
  }

  void changeValue()
  {
    _currVal = !_currVal;
    featureService.set(PUSH_TO_START, _currVal);
  }

private:
  char *_text;
  bool _currVal;
};

template <typename T>
class GenericOptionHandler : public SettingHandlerBase
{
  typedef const char *(*GetCallback)();
  typedef void (*SetCallback)();

public:
  GenericOptionHandler(char *title, GetCallback getCallback, SetCallback setCallback)
  {
    _title = title;
    _getCallback = getCallback;
    _setCallback = setCallback;
    _currVal = getCallback();
  }

  void display()
  {
    Serial.printf("%s changed value to %s\n", _title, _currVal);
    screenPropValue<T>(_title, _currVal);
  }

  void changeValue()
  {
    if (_setCallback != nullptr)
    {
      _setCallback();
      _currVal = _getCallback();
    }
    else
      Serial.printf("WARNING: setCallback not set in GenericOptionHandler (%s)\n", _title);
  }

private:
  char *_title;
  const char *_currVal;
  GetCallback _getCallback = nullptr;
  SetCallback _setCallback = nullptr;
};

enum SettingOption
{
  OPT_PUSH_TO_START = 0,
  OPT_SECOND_SETTING,
  OPT_THIRD_SETTING,
  OPT_Length,
};

uint16_t settingPtr = PUSH_TO_START;

uint8_t secondOptionVal = 0;
const char *getSecondOption()
{
  static char buff[4];
  sprintf(buff, "%d", secondOptionVal);
  return buff;
}

uint8_t thirdOptionVal = 0;
const char *getThirdOption()
{
  static char buff[4];
  sprintf(buff, "%d", thirdOptionVal);
  return buff;
}

PushToStart pushToStartHandler("push to start");

GenericOptionHandler<uint8_t> secondOptionHandler(
    "second option",
    getSecondOption,
    []() { secondOptionVal++; });

GenericOptionHandler<uint8_t> thirdOptionHandler(
    "third option",
    getThirdOption,
    []() { thirdOptionVal++; });

// array of the options
SettingHandlerBase *optionHandlers[] = {
    &pushToStartHandler,
    &secondOptionHandler,
    &thirdOptionHandler};

void nextSetting()
{
  settingPtr++;
  settingPtr = settingPtr == OPT_Length
                   ? SettingOption(0)
                   : SettingOption(settingPtr);
}