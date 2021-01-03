#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
#endif

namespace Display
{
  bool update_display = false;
  DispState::Trigger lastDispEvent;

  elapsedMillis sinceShowingToggleScreen;
  elapsedMillis sinceStoredMovingTime;

  enum StateId
  {
    SEARCHING = 0,
    DISCONNECTED,
    STOPPED_SCREEN,
    MOVING_SCREEN,
    NEED_TO_ACK_RESETS_STOPPED,
    NEED_TO_ACK_RESETS_MOVING,
    BOARD_VERSION_DOESNT_MATCH_SCREEN,
    SHOW_PUSH_TO_START,
    SHOW_SETTINGS,
    TOGGLE_PUSH_TO_START,
  };

  const char *stateID(uint16_t id)
  {
    switch (id)
    {
    case SEARCHING:
      return "SEARCHING";
    case DISCONNECTED:
      return "DISCONNECTED";
    case STOPPED_SCREEN:
      return "STOPPED_SCREEN";
    case MOVING_SCREEN:
      return "MOVING_SCREEN";
    case NEED_TO_ACK_RESETS_STOPPED:
      return "NEED_TO_ACK_RESETS_STOPPED";
    case NEED_TO_ACK_RESETS_MOVING:
      return "NEED_TO_ACK_RESETS_MOVING";
    case BOARD_VERSION_DOESNT_MATCH_SCREEN:
      return "BOARD_VERSION_DOESNT_MATCH_SCREEN";
    case SHOW_PUSH_TO_START:
      return "SHOW_PUSH_TO_START";
    case SHOW_SETTINGS:
      return "SHOW_SETTINGS";
    case TOGGLE_PUSH_TO_START:
      return "TOGGLE_PUSH_TO_START";
    }
    return outOfRange("Display::stateID()");
  }

  FsmManager<DispState::Trigger> dispFsm;

  //---------------------------------------------------------------
  State stateSearching([] {
    dispFsm.printState(SEARCHING);
    screen_searching();
  });
  //---------------------------------------------------------------
  State stateDisconnected(
      [] {
        dispFsm.printState(DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateStoppedScreen(
      [] {
        dispFsm.printState(STOPPED_SCREEN);
        screenWhenStopped(/*init*/ true);
      },
      [] {
        if (update_display || stats.update)
        {
          update_display = false;
          stats.update = false;
          screenWhenStopped(/*init*/ false);
          if (FEATURE_SEND_TO_HUD)
            updateHudIcon(hudClient.connected());
        }
      },
      NULL);
  //---------------------------------------------------------------
  State stateMovingScreen(
      [] {
        dispFsm.printState(MOVING_SCREEN);
        sinceStoredMovingTime = 0;
        screenWhenMoving(/*init*/ true);
      },
      [] {
        if (update_display || stats.update)
        {
          update_display = false;
          stats.update = false;
          screenWhenMoving(/*init*/ false);
          if (FEATURE_SEND_TO_HUD)
            updateHudIcon(hudClient.connected());
        }

        stats.addMovingTime(sinceStoredMovingTime);
        sinceStoredMovingTime = 0;
      },
      NULL);
  //---------------------------------------------------------------
  State stateNeedToAckResetsStopped(
      [] {
        dispFsm.printState(NEED_TO_ACK_RESETS_STOPPED);
        screenNeedToAckResets();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateNeedToAckResetsMoving(
      [] {
        dispFsm.printState(NEED_TO_ACK_RESETS_MOVING);
        sinceStoredMovingTime = 0;
        screenNeedToAckResets();
      },
      [] {
        stats.addMovingTime(sinceStoredMovingTime);
        sinceStoredMovingTime = 0;
      },
      NULL);
  //---------------------------------------------------------------
  State stateBoardVersionDoesntMatchScreen(
      [] {
        dispFsm.printState(BOARD_VERSION_DOESNT_MATCH_SCREEN);
        screenBoardNotCompatible(board.packet.version);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

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
  //---------------------------------------------------------------
  State stShowSettings(
      [] {
        dispFsm.printState(SHOW_SETTINGS);
        sinceShowingToggleScreen = 0;
        switch (lastDispEvent)
        {
        case DispState::PRIMARY_TRIPLE_CLICK:
        {
          settingPtr = SettingOption::OPT_PUSH_TO_START;
          optionHandlers[settingPtr]->display();
          break;
        }
        case DispState::PRIMARY_SINGLE_CLICK:
        {
          nextSetting();
          optionHandlers[settingPtr]->display();
          break;
        }
        case DispState::PRIMARY_LONG_PRESS:
        {
          optionHandlers[settingPtr]->changeValue();
          optionHandlers[settingPtr]->display();
          break;
        }
        }
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateShowPushToStart(
      [] {
        dispFsm.printState(SHOW_PUSH_TO_START);
        bool currVal = featureService.get<bool>(FeatureType::PUSH_TO_START);
        screenPropValue<bool>("push to start", (currVal == true) ? "ON" : "OFF");

        if (lastDispEvent == DispState::PRIMARY_LONG_PRESS)
        {
          currVal = !currVal;
          featureService.set(PUSH_TO_START, currVal);
        }
        else
        {
          Serial.printf("lastEvent was %s\n", DispState::getTrigger(lastDispEvent));
        }
        screenPropValue<bool>("push to start", (currVal == true) ? "ON" : "OFF");
        sinceShowingToggleScreen = 0;
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  void acknowledgeResets()
  {
    stats.ackResets();
  }
  //---------------------------------------------------------------

  Fsm fsm(&stateSearching);

  void addTransitions()
  {
    // DispState::CONNECTED
    fsm.add_transition(&stateSearching, &stateStoppedScreen, DispState::CONNECTED, NULL);
    fsm.add_transition(&stateDisconnected, &stateStoppedScreen, DispState::CONNECTED, NULL);

    // DispState::DISCONNECTED
    fsm.add_transition(&stateStoppedScreen, &stateDisconnected, DispState::DISCONNECTED, NULL);
    fsm.add_transition(&stateMovingScreen, &stateDisconnected, DispState::DISCONNECTED, NULL);

    // DispState::SW_RESET
    fsm.add_transition(&stateStoppedScreen, &stateNeedToAckResetsStopped, DispState::SW_RESET, NULL);
    fsm.add_transition(&stateMovingScreen, &stateNeedToAckResetsMoving, DispState::SW_RESET, NULL);

    fsm.add_transition(&stateNeedToAckResetsStopped, &stateStoppedScreen, DispState::PRIMARY_DOUBLE_CLICK, acknowledgeResets);
    fsm.add_transition(&stateNeedToAckResetsStopped, &stateNeedToAckResetsMoving, DispState::MOVING, NULL);
    fsm.add_transition(&stateNeedToAckResetsMoving, &stateNeedToAckResetsStopped, DispState::STOPPED, NULL);
    fsm.add_transition(&stateNeedToAckResetsMoving, &stateMovingScreen, DispState::PRIMARY_DOUBLE_CLICK, acknowledgeResets);

    // DispState::MOVING
    fsm.add_transition(&stateStoppedScreen, &stateMovingScreen, DispState::MOVING, NULL);
    // DispState::STOPPED
    fsm.add_transition(&stateMovingScreen, &stateStoppedScreen, DispState::STOPPED, NULL);

    // Push to start
    fsm.add_transition(&stateStoppedScreen, &stShowSettings, DispState::PRIMARY_TRIPLE_CLICK, NULL);
    fsm.add_timed_transition(&stShowSettings, &stateStoppedScreen, 3000, NULL);
    fsm.add_transition(&stShowSettings, &stShowSettings, DispState::PRIMARY_SINGLE_CLICK, NULL);
    fsm.add_transition(&stShowSettings, &stShowSettings, DispState::PRIMARY_LONG_PRESS, NULL);

    // fsm.add_transition(&stateStoppedScreen, &stateShowPushToStart, DispState::PRIMARY_TRIPLE_CLICK, NULL);
    // fsm.add_timed_transition(&stateShowPushToStart, &stateStoppedScreen, 3000, NULL);
    // fsm.add_transition(&stateShowPushToStart, &stateShowPushToStart, DispState::PRIMARY_LONG_PRESS, NULL);
    // fsm.add_transition(&stateShowPushToStart, &stateStoppedScreen, DispState::PRIMARY_SINGLE_CLICK, NULL);

    // fsm.add_transition(&stateStoppedScreen, &stateTogglePushToStart, DispState::PRIMARY_TRIPLE_CLICK, NULL);
    // fsm.add_transition(&stateTogglePushToStart, &stateTogglePushToStart, DispState::PRIMARY_LONG_PRESS, NULL);
    // fsm.add_transition(&stateTogglePushToStart, &stateStoppedScreen, DispState::PRIMARY_SINGLE_CLICK, NULL);

    // DispState::UPDATE
    fsm.add_transition(&stateStoppedScreen, &stateStoppedScreen, DispState::UPDATE, NULL);
    fsm.add_transition(&stateMovingScreen, &stateMovingScreen, DispState::UPDATE, NULL);

    // DispState::VERSION_DOESNT_MATCH
    fsm.add_transition(&stateStoppedScreen, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
    fsm.add_transition(&stateMovingScreen, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  }
} // namespace Display