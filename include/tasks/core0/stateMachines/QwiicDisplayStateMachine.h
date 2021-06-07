#include <shared-utils.h>
#include <FsmManager.h>
#include <utils.h>

namespace nsQwiicDisplayTask
{
  enum Event
  {
    EV_NO_EVENT = 0,
    EV_OFFLINE,
    EV_STOPPED,
    EV_MOVING,
  };

  const char *getEvent(uint16_t ev)
  {
    switch (ev)
    {
    case EV_NO_EVENT:
      return "EV_NO_EVENT";
    case EV_OFFLINE:
      return "EV_OFFLINE";
    case EV_STOPPED:
      return "EV_STOPPED";
    case EV_MOVING:
      return "EV_MOVING";
    }
    return getOutOfRange("QwiicDisplay::getEvent()");
  }

  Event lastEvent;

  enum StateId
  {
    ST_OFFLINE,
    ST_STOPPED,
    ST_BOARD_BATT,
    ST_REMOTE_BATT,
    ST_MOVING,
  };

  const char *getStateId(uint16_t id)
  {
    switch (id)
    {
    case ST_OFFLINE:
      return "ST_OFFLINE";
    case ST_STOPPED:
      return "ST_STOPPED";
    case ST_BOARD_BATT:
      return "ST_BOARD_BATT";
    case ST_REMOTE_BATT:
      return "ST_REMOTE_BATT";
    case ST_MOVING:
      return "ST_MOVING";
    }
    return getOutOfRange("QwiicDisplay::getStateId()");
  }

  FsmManager<Event> fsm_mgr;

  void printState(uint16_t id)
  {
    Serial.printf(PRINT_FSM_STATE_FORMAT, "QwiicDisplayTask", millis(), getStateId(id));
  }

  void printTrigger(uint16_t ev)
  {
    Serial.printf(PRINT_FSM_TRIGGER_FORMAT, "QwiicDisplayTask", millis(), getEvent(ev));
  }

  //---------------------------------------------------------------
  State stateOffline(
      ST_OFFLINE,
      []
      {
        fsm_mgr.printState(ST_OFFLINE);
        centreMessage("Offline");
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stStopped(
      ST_STOPPED,
      []
      {
        if (fsm_mgr.prevState() != ST_BOARD_BATT &&
            fsm_mgr.prevState() != ST_REMOTE_BATT)
          fsm_mgr.printState(ST_STOPPED);

        centreMessage("Stopped");
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stBoardBattery(
      ST_BOARD_BATT,
      []
      {
        char buff[10];
        sprintf(buff, "%d%%", getBatteryPercentage(g_BoardBatteryVolts));
        valueMessage("Board:", buff);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stRemoteBattery(
      ST_BOARD_BATT,
      []
      {
        valueMessage("Remote:", "100%");
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stMoving(
      nsQwiicDisplayTask::ST_MOVING,
      []
      {
        // if (fsm_mgr.lastEvent() == EV_MOVING)
        //   return;
        fsm_mgr.printState(nsQwiicDisplayTask::ST_MOVING);
        centreMessage("Moving");
      },
      NULL,
      NULL);

  Fsm _fsm(&stateOffline);

  void addTransitions()
  {
    // EV_OFFLINE
    _fsm.add_transition(&stStopped, &stateOffline, EV_OFFLINE, NULL);
    _fsm.add_transition(&stMoving, &stateOffline, EV_OFFLINE, NULL);

    // EV_STOPPED
    _fsm.add_transition(&stateOffline, &stStopped, EV_STOPPED, NULL);
    _fsm.add_transition(&stMoving, &stStopped, EV_STOPPED, NULL);

    // EV_MOVING
    _fsm.add_transition(&stStopped, &stMoving, EV_MOVING, NULL);
    _fsm.add_transition(&stBoardBattery, &stMoving, EV_MOVING, NULL);
    _fsm.add_transition(&stRemoteBattery, &stMoving, EV_MOVING, NULL);

    // timed
    _fsm.add_timed_transition(&stStopped, &stBoardBattery, 1000, NULL);
    _fsm.add_timed_transition(&stBoardBattery, &stRemoteBattery, 2000, NULL);
    _fsm.add_timed_transition(&stRemoteBattery, &stStopped, 2000, NULL);
  }
}