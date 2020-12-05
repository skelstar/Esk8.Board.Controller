#ifndef Fsm
#include <Fsm.h>
#endif

namespace CommsEvent
{
  enum Event
  {
    NO_EVENT,
    PKT_RXD,
    BOARD_TIMEDOUT,
    BD_FIRST_PACKET,
    Length,
  };

  const char *names[] = {
      "NO_EVENT",
      "PKT_RXD",
      "BOARD_TIMEDOUT",
      "BD_FIRST_PACKET",
      "Length",
  };

  bool outOfRange(uint8_t ev)
  {
    return ev >= Event::Length;
  }

  const char *getName(uint8_t ev)
  {
    if (!outOfRange(ev))
      return names[ev];
    return "WARNING: OUT OF RANGE";
  }

  bool isValid(uint8_t ev)
  {
    return !outOfRange(ev) && ev != NO_EVENT;
  }

  void print(uint8_t ev)
  {
    Serial.printf("--> CommsEvent: %s\n", outOfRange(ev)
                                              ? "OUT OF RANGE"
                                              : names[ev]);
  }
} // namespace CommsEvent
//------------------------------------------

/* prototypes */
CommsEvent::Event readFromCommsStateEventQueue(TickType_t ticks = 5);
bool boardVersionCompatible(float version);
//------------------------------------------

bool comms_session_started = false;
bool comms_state_connected = false;
bool commsStateTask_initialised = false;

bool skipOnEnter = false;

CommsEvent::Event lastCommsEvent = CommsEvent::NO_EVENT;

Fsm *commsFsm;

/* prototypes */

//------------------------------------------
State stateCommsSearching([] {
#ifdef PRINT_COMMS_STATE
  commsFsm->print("stateCommsSearching", false);
#endif
});

elapsedMillis sinceReported;

State stateCommsConnected(
    [] {
#ifdef PRINT_COMMS_STATE
      commsFsm->print("stateCommsConnected", false);
#endif
      if (commsFsm->lastEvent() == CommsEvent::BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DISP_EV_UPDATE);
      }

      comms_session_started = true;
      comms_state_connected = true;

      displayChangeQueueManager->send(DISP_EV_CONNECTED);
      displayChangeQueueManager->send(DISP_EV_UPDATE);

      if (stats.needToAckResets())
      {
        displayChangeQueueManager->send(DISP_EV_SW_RESET);
      }

      // check board version is compatible
      bool boardCompatible = boardVersionCompatible(board.packet.version);
      if (!boardCompatible)
      {
        displayChangeQueueManager->send(DISP_EV_VERSION_DOESNT_MATCH);
      }
    },
    NULL,
    NULL);

State stateCommsDisconnected(
    [] {
      if (commsFsm->lastEvent() == CommsEvent::BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DISP_EV_UPDATE);
      }

#ifdef PRINT_COMMS_STATE
      commsFsm->print("stateCommsDisconnected");
      DEBUGVAL(board.sinceLastPacket);
#endif

      comms_state_connected = false;
      displayChangeQueueManager->send(DISP_EV_DISCONNECTED);
    },
    NULL, NULL);
//-----------------------------------------------------

void addCommsStateTransitions()
{
  // CommsEvent::PKT_RXD
  commsFsm->add_transition(&stateCommsSearching, &stateCommsConnected, CommsEvent::PKT_RXD, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsConnected, CommsEvent::PKT_RXD, NULL);

  // CommsEvent::BOARD_TIMEDOUT
  commsFsm->add_transition(&stateCommsConnected, &stateCommsDisconnected, CommsEvent::BOARD_TIMEDOUT, NULL);

  // CommsEvent::BD_RESET
  commsFsm->add_transition(&stateCommsConnected, &stateCommsConnected, CommsEvent::BD_FIRST_PACKET, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsDisconnected, CommsEvent::BD_FIRST_PACKET, NULL);
}
//-----------------------------------------------------

void triggerCommsEvent(CommsEvent::Event ev)
{
#ifdef PRINT_COMMS_STATE_EVENT
  if (ev == CommsEvent::PKT_RXD)
  {
    // CommsEvent::print(ev);
  }
  else
  {
    CommsEvent::print(ev);
  }
#endif
  lastCommsEvent = ev;
  commsFsm->trigger(ev);
}

//-------------------------------------------------

void commsStateTask_0(void *pvParameters)
{

  Serial.printf("commsStateTask_0 running on core %d\n", xPortGetCoreID());

  commsStateTask_initialised = true;

  commsFsm = new Fsm(&stateCommsSearching);

  addCommsStateTransitions();

  while (false == display_task_initialised)
  {
    vTaskDelay(1);
  }

  while (true)
  {
    uint8_t ev = commsEventQueue->read();
    if (CommsEvent::isValid(ev))
    {
      triggerCommsEvent((CommsEvent::Event)ev);
    }

    commsFsm->run_machine();

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
