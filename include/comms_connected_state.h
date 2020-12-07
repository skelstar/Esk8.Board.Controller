#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef ENUMMANAGER_H
#include <EnumManager.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
#endif

FsmManager commsFsmManager;

bool comms_session_started = false;
bool comms_state_connected = false;
bool commsStateTask_initialised = false;

//------------------------------------------

/* prototypes */
bool boardVersionCompatible(float version);

//------------------------------------------

elapsedMillis sinceReported;

namespace Comms
{
  enum StateID
  {
    SEARCHING = 0,
    CONNECTED,
    DISCONNECTED,
    StateIDLength,
  };

  std::string stateIDName[] = {
      "SEARCHING",
      "CONNECTED",
      "DISCONNECTED",
  };

  enum Event
  {
    NO_EVENT,
    PKT_RXD,
    BOARD_TIMEDOUT,
    BD_FIRST_PACKET,
    EventLength,
  };

  std::string eventNames[] = {
      "NO_EVENT",
      "PKT_RXD",
      "BOARD_TIMEDOUT",
      "BD_FIRST_PACKET",
  };

  EnumManager<Event> eventMgr(eventNames);
  EnumManager<StateID> stateIdMgr(stateIDName);

  void print(uint8_t ev)
  {
    Serial.printf("--> CommsEvent: %s\n", eventMgr.getName(ev));
  }

  State stateCommsSearching(
      SEARCHING,
      [] {
#ifdef PRINT_COMMS_STATE
        commsFsmManager.printState(SEARCHING);
#endif
      },
      [] {},
      [] {});

  State stateCommsConnected(
      CONNECTED,
      [] {
#ifdef PRINT_COMMS_STATE
        commsFsmManager.printState(CONNECTED);
#endif
        if (commsFsmManager.lastEvent() == Comms::BD_FIRST_PACKET)
        {
          stats.boardResets++;
          displayChangeQueueManager->send(Disp::UPDATE);
        }

        comms_session_started = true;
        comms_state_connected = true;

        displayChangeQueueManager->send(Disp::CONNECTED);
        displayChangeQueueManager->send(Disp::UPDATE);

        if (stats.needToAckResets())
        {
          displayChangeQueueManager->send(Disp::SW_RESET);
        }

        // check board version is compatible
        bool boardCompatible = boardVersionCompatible(board.packet.version);
        if (!boardCompatible)
        {
          displayChangeQueueManager->send(Disp::VERSION_DOESNT_MATCH);
        }
      },
      NULL,
      NULL);

  State stateCommsDisconnected(
      DISCONNECTED,
      [] {
#ifdef PRINT_COMMS_STATE
        commsFsmManager.printState(DISCONNECTED);
#endif

        if (commsFsmManager.lastEvent() == Comms::BD_FIRST_PACKET)
        {
          stats.boardResets++;
          displayChangeQueueManager->send(Disp::UPDATE);
        }

        comms_state_connected = false;
        displayChangeQueueManager->send(Disp::DISCONNECTED);
      },
      NULL, NULL);
  //-----------------------------------------------------
  /* prototypes */

  Fsm fsm(&stateCommsSearching);

  void addTransitions()
  {
    // Comms::PKT_RXD
    fsm.add_transition(&stateCommsSearching, &stateCommsConnected, Comms::PKT_RXD, NULL);
    fsm.add_transition(&stateCommsDisconnected, &stateCommsConnected, Comms::PKT_RXD, NULL);

    // Comms::BOARD_TIMEDOUT
    fsm.add_transition(&stateCommsConnected, &stateCommsDisconnected, Comms::BOARD_TIMEDOUT, NULL);

    // Comms::BD_RESET
    fsm.add_transition(&stateCommsConnected, &stateCommsConnected, Comms::BD_FIRST_PACKET, NULL);
    fsm.add_transition(&stateCommsDisconnected, &stateCommsDisconnected, Comms::BD_FIRST_PACKET, NULL);
  }
} // namespace Comms

//------------------------------------------

bool skipOnEnter = false;

Comms::Event lastCommsEvent = Comms::NO_EVENT;

void triggerCommsEvent(Comms::Event ev)
{
#ifdef PRINT_COMMS_STATE_EVENT
  if (ev == Comms::PKT_RXD)
  {
    // Comms::print(ev);
  }
  else
  {
    Comms::print(ev);
  }
#endif
  lastCommsEvent = ev;
  commsFsmManager.trigger(ev);
}

//-------------------------------------------------

void commsStateTask_0(void *pvParameters)
{
  RTOSUtils::printTaskDetails();

  commsStateTask_initialised = true;

  commsFsmManager.begin(&Comms::fsm, ">State: %s | %s\n");
  commsFsmManager.setGetEventNameCallback([](uint8_t ev) {
    return Comms::eventMgr.getName(ev); // "WARNING: OUT OF RANGE";
  });
  commsFsmManager.setGetStateNameCallback([](uint8_t id) {
    return Comms::stateIdMgr.getName(id);
  });

  Comms::addTransitions();

  while (false == display_task_initialised)
  {
    vTaskDelay(1);
  }

  while (true)
  {
    uint8_t ev = commsEventQueue->read();
    if (Comms::eventMgr.isValid(ev))
    {
      commsFsmManager.trigger(ev);
    }

    Comms::fsm.run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
// check version
bool boardVersionCompatible(float version)
{
  return version == VERSION_BOARD_COMPAT;
}
