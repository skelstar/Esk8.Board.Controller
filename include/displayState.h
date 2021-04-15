
#define LCD_WIDTH 240
#define LCD_HEIGHT 135

#define TFT_DEFAULT_BG TFT_BLACK
//------------------------------------------------------------

// BatteryInfo *remote;

#include <FsmManager.h>
#include <screens.h>
#include <FeatureService.h>
#include <tasks/core0/statsTask.h>

//------------------------------------------------------------

namespace DispState
{
  enum Trigger
  {
    TR_NO_EVENT = 0,
    TR_CONNECTED,
    TR_DISCONNECTED,
    TR_STOPPED,
    TR_MOVING,
    TR_UPDATE,
    TR_REMOTE_BATTERY_CHANGED,
    TR_SELECT_BUTTON_CLICK,
    TR_VERSION_DOESNT_MATCH,
    TR_MENU_BUTTON_CLICKED,
    TR_OPTION_TIMED_OUT,
  };

  const char *getTrigger(int ev)
  {
    switch (ev)
    {
    case TR_NO_EVENT:
      return "TR_NO_EVENT";
    case TR_CONNECTED:
      return "TR_CONNECTED";
    case TR_DISCONNECTED:
      return "TR_DISCONNECTED";
    case TR_STOPPED:
      return "TR_STOPPED";
    case TR_MOVING:
      return "TR_MOVING";
    case TR_UPDATE:
      return "TR_UPDATE";
    case TR_REMOTE_BATTERY_CHANGED:
      return "TR_REMOTE_BATTERY_CHANGED";
    case TR_SELECT_BUTTON_CLICK:
      return "TR_SELECT_BUTTON_CLICK";
    case TR_VERSION_DOESNT_MATCH:
      return "TR_VERSION_DOESNT_MATCH";
    case TR_MENU_BUTTON_CLICKED:
      return "TR_MENU_BUTTON_CLICKED";
    case TR_OPTION_TIMED_OUT:
      return "TR_OPTION_TIMED_OUT";
    }
    return OUT_OF_RANGE;
  }
} // namespace DispState

namespace Display
{
  DispState::Trigger lastDispEvent;

  elapsedMillis sinceShowingOptionScreen;

  enum StateId
  {
    ST_START_UP,
    ST_DISCONNECTED,
    ST_SOFTWARE_STATS,
    ST_BOARD_BATTERY,
    ST_STOPPED_SCREEN,
    ST_MOVING_SCREEN,
    ST_BOARD_VERSION_DOESNT_MATCH_SCREEN,
    ST_OPTION_PUSH_TO_START,
    ST_SHOW_SETTINGS,
    ST_TOGGLE_PUSH_TO_START,
  };

  const char *stateID(uint16_t id)
  {
    switch (id)
    {
    case ST_START_UP:
      return "ST_START_UP";
    case ST_DISCONNECTED:
      return "ST_DISCONNECTED";
    case ST_SOFTWARE_STATS:
      return "ST_SOFTWARE_STATS";
    case ST_BOARD_BATTERY:
      return "ST_BOARD_BATTERY";
    case ST_STOPPED_SCREEN:
      return "ST_STOPPED_SCREEN";
    case ST_MOVING_SCREEN:
      return "ST_MOVING_SCREEN";
    case ST_BOARD_VERSION_DOESNT_MATCH_SCREEN:
      return "ST_BOARD_VERSION_DOESNT_MATCH_SCREEN";
    case ST_OPTION_PUSH_TO_START:
      return "ST_OPTION_PUSH_TO_START";
    case ST_SHOW_SETTINGS:
      return "ST_SHOW_SETTINGS";
    case ST_TOGGLE_PUSH_TO_START:
      return "ST_TOGGLE_PUSH_TO_START";
    }
    return "OUT OF RANGE: Display::stateID()";
  }

  FsmManager<DispState::Trigger> fsm_mgr;

  //---------------------------------------------------------------
  State stateDisconnected(
      ST_DISCONNECTED,
      [] {
        // TODO: make a param-less version of printState (i.e. if id set)
        fsm_mgr.printState(ST_DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stBoardBattery(
      ST_BOARD_BATTERY,
      [] {
        fsm_mgr.printState(ST_BOARD_BATTERY);
        // screenBoardBattery(board.packet.batteryVoltage);
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stStopped(
      ST_STOPPED_SCREEN,
      [] {
        // don't need TR_UPDATE at this stage
        if (fsm_mgr.lastEvent() == DispState::TR_UPDATE)
          return;

        fsm_mgr.printState(ST_STOPPED_SCREEN);

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
      ST_MOVING_SCREEN,
      [] {
        // don't need UPDATE at this stage
        if (fsm_mgr.lastEvent() == DispState::TR_UPDATE)
          return;

        fsm_mgr.printState(ST_MOVING_SCREEN);

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
      ST_BOARD_VERSION_DOESNT_MATCH_SCREEN,
      [] {
        fsm_mgr.printState(ST_BOARD_VERSION_DOESNT_MATCH_SCREEN);
        // screenBoardNotCompatible(board.packet.version);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

#define OPTION_SCREEN_TIMEOUT 2000

  State stOptionPushToStart(
      ST_OPTION_PUSH_TO_START,
      [] {
        fsm_mgr.printState(ST_OPTION_PUSH_TO_START);
        sinceShowingOptionScreen = 0;
        switch (fsm_mgr.lastEvent())
        {
        case DispState::TR_MENU_BUTTON_CLICKED:
        {
          bool enabled = featureService.get<bool>(FeatureType::PUSH_TO_START);
          screenPropValue<bool>("Push to start", enabled ? "ON" : "OFF");
          break;
        }
        case DispState::TR_SELECT_BUTTON_CLICK:
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
          fsm_mgr.trigger(DispState::TR_OPTION_TIMED_OUT);
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
    // ST_DISCONNECTED
    _fsm.add_transition(&stStopped, &stateDisconnected, DispState::TR_DISCONNECTED, NULL);
    _fsm.add_transition(&stMoving, &stateDisconnected, DispState::TR_DISCONNECTED, NULL);

    // Options
    _fsm.add_transition(&stStopped, &stOptionPushToStart, DispState::TR_MENU_BUTTON_CLICKED, NULL);
    _fsm.add_transition(&stOptionPushToStart, &stStopped, DispState::TR_MENU_BUTTON_CLICKED, NULL);
    _fsm.add_transition(&stOptionPushToStart, &stOptionPushToStart, DispState::TR_SELECT_BUTTON_CLICK, NULL);
    _fsm.add_transition(&stOptionPushToStart, &stStopped, DispState::TR_OPTION_TIMED_OUT, NULL);

    // TR_MOVING
    _fsm.add_transition(&stateDisconnected, &stMoving, DispState::TR_MOVING, NULL);
    _fsm.add_transition(&stStopped, &stMoving, DispState::TR_MOVING, NULL);

    // TR_STOPPED
    _fsm.add_transition(&stateDisconnected, &stStopped, DispState::TR_STOPPED, NULL);
    _fsm.add_transition(&stMoving, &stStopped, DispState::TR_STOPPED, NULL);

    // TR_REMOTE_BATTERY_CHANGED
    _fsm.add_transition(&stStopped, &stStopped, DispState::TR_REMOTE_BATTERY_CHANGED, NULL);
    _fsm.add_transition(&stateDisconnected, &stateDisconnected, DispState::TR_REMOTE_BATTERY_CHANGED, NULL);

    // UPDATE
    _fsm.add_transition(&stStopped, &stStopped, DispState::TR_UPDATE, NULL);
    _fsm.add_transition(&stMoving, &stMoving, DispState::TR_UPDATE, NULL);

    // TR_VERSION_DOESNT_MATCH
    _fsm.add_transition(&stateDisconnected, &stBoardVersionDoesntMatchScreen, DispState::TR_VERSION_DOESNT_MATCH, NULL);
  }
} // namespace Display
