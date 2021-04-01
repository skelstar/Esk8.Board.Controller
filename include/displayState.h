namespace Display
{
  DispState::Trigger lastDispEvent;

  elapsedMillis sinceShowingOptionScreen;

  enum StateId
  {
    START_UP,
    DISCONNECTED,
    SOFTWARE_STATS,
    BOARD_BATTERY,
    STOPPED_SCREEN,
    MOVING_SCREEN,
    BOARD_VERSION_DOESNT_MATCH_SCREEN,
    OPTION_PUSH_TO_START,
    SHOW_SETTINGS,
    TOGGLE_PUSH_TO_START,
  };

  const char *stateID(uint16_t id)
  {
    switch (id)
    {
    case START_UP:
      return "START_UP";
    case DISCONNECTED:
      return "DISCONNECTED";
    case SOFTWARE_STATS:
      return "SOFTWARE_STATS";
    case BOARD_BATTERY:
      return "BOARD_BATTERY";
    case STOPPED_SCREEN:
      return "STOPPED_SCREEN";
    case MOVING_SCREEN:
      return "MOVING_SCREEN";
    case BOARD_VERSION_DOESNT_MATCH_SCREEN:
      return "BOARD_VERSION_DOESNT_MATCH_SCREEN";
    case OPTION_PUSH_TO_START:
      return "OPTION_PUSH_TO_START";
    case SHOW_SETTINGS:
      return "SHOW_SETTINGS";
    case TOGGLE_PUSH_TO_START:
      return "TOGGLE_PUSH_TO_START";
    }
    return outOfRange("Display::stateID()");
  }

  FsmManager<DispState::Trigger> fsm_mgr;

  //---------------------------------------------------------------
  State stateDisconnected(
      DISCONNECTED,
      [] {
        // TODO: make a param-less version of printState (i.e. if id set)
        fsm_mgr.printState(DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stBoardBattery(
      BOARD_BATTERY,
      [] {
        fsm_mgr.printState(BOARD_BATTERY);
        screenBoardBattery(board.packet.batteryVoltage);
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stStopped(
      DispState::STOPPED,
      [] {
        // don't need UPDATE at this stage
        if (fsm_mgr.lastEvent() == DispState::UPDATE)
          return;

        fsm_mgr.printState(STOPPED_SCREEN);

        if (stats.controllerResets > 0)
          screenNeedToAckResets(Stats::CONTROLLER_RESETS);
        else if (stats.boardResets > 0)
          screenNeedToAckResets(Stats::BOARD_RESETS);
        else
          simpleStoppedScreen("STOPPED", TFT_CASET);
        // screenWhenStopped(/*init*/ true);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stMoving(
      DispState::MOVING,
      [] {
        // don't need UPDATE at this stage
        if (fsm_mgr.lastEvent() == DispState::UPDATE)
          return;

        fsm_mgr.printState(MOVING_SCREEN);

        if (stats.controllerResets > 0)
          screenNeedToAckResets(Stats::CONTROLLER_RESETS);
        else if (stats.boardResets > 0)
          screenNeedToAckResets(Stats::BOARD_RESETS);
        else
          simpleMovingScreen();
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stBoardVersionDoesntMatchScreen(
      BOARD_VERSION_DOESNT_MATCH_SCREEN,
      [] {
        fsm_mgr.printState(BOARD_VERSION_DOESNT_MATCH_SCREEN);
        screenBoardNotCompatible(board.packet.version);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

#define OPTION_SCREEN_TIMEOUT 2000

  State stOptionPushToStart(
      OPTION_PUSH_TO_START,
      [] {
        fsm_mgr.printState(OPTION_PUSH_TO_START);
        sinceShowingOptionScreen = 0;
        switch (fsm_mgr.lastEvent())
        {
        case DispState::MENU_BUTTON_CLICKED:
        {
          bool enabled = featureService.get<bool>(FeatureType::PUSH_TO_START);
          screenPropValue<bool>("Push to start", enabled ? "ON" : "OFF");
          break;
        }
        case DispState::SELECT_BUTTON_CLICK:
        {
          sinceShowingOptionScreen = 0;
          bool enabled = featureService.get<bool>(FeatureType::PUSH_TO_START);
          featureService.set(PUSH_TO_START, !enabled);
          screenPropValue<bool>("Push to start", !enabled ? "ON" : "OFF");
          break;
        }
        default:
          Serial.printf("Unhandled event: %s\n", DispState::getTrigger(fsm_mgr.lastEvent()));
        }
      },
      [] {
        if (sinceShowingOptionScreen > OPTION_SCREEN_TIMEOUT)
          fsm_mgr.trigger(DispState::OPTION_TIMED_OUT);
      },
      NULL);
  //---------------------------------------------------------------

  Fsm _fsm(&stateDisconnected);

  void clearResetCounters()
  {
    if (stats.controllerResets > 0)
      stats.clearControllerResets();
    else if (stats.boardResets > 0)
      stats.clearBoardResets();
  }

  void addTransitions()
  {
    // DISCONNECTED
    _fsm.add_transition(&stStopped, &stateDisconnected, DispState::DISCONNECTED, NULL);
    _fsm.add_transition(&stMoving, &stateDisconnected, DispState::DISCONNECTED, NULL);

    // Options
    _fsm.add_transition(&stStopped, &stOptionPushToStart, DispState::MENU_BUTTON_CLICKED, NULL);
    _fsm.add_transition(&stOptionPushToStart, &stStopped, DispState::MENU_BUTTON_CLICKED, NULL);
    _fsm.add_transition(&stOptionPushToStart, &stOptionPushToStart, DispState::SELECT_BUTTON_CLICK, NULL);
    _fsm.add_transition(&stOptionPushToStart, &stStopped, DispState::OPTION_TIMED_OUT, NULL);

    // MOVING
    _fsm.add_transition(&stateDisconnected, &stMoving, DispState::MOVING, NULL);
    _fsm.add_transition(&stStopped, &stMoving, DispState::MOVING, NULL);

    // STOPPED
    _fsm.add_transition(&stateDisconnected, &stStopped, DispState::STOPPED, NULL);
    _fsm.add_transition(&stMoving, &stStopped, DispState::STOPPED, NULL);

    // REMOTE_BATTERY_CHANGED
    _fsm.add_transition(&stStopped, &stStopped, DispState::REMOTE_BATTERY_CHANGED, NULL);

    // UPDATE
    _fsm.add_transition(&stStopped, &stStopped, DispState::UPDATE, NULL);
    _fsm.add_transition(&stMoving, &stMoving, DispState::UPDATE, NULL);

    // VERSION_DOESNT_MATCH
    _fsm.add_transition(&stateDisconnected, &stBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  }
} // namespace Display
