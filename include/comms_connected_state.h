#ifndef Fsm
#include <Fsm.h>
#endif

namespace CommsState
{
  enum Event
  {
    EV_COMMS_NO_EVENT,
    EV_COMMS_PKT_RXD,
    EV_COMMS_BOARD_TIMEDOUT,
    EV_COMMS_BD_FIRST_PACKET,
    EV_COMMS_Length,
  };

  const char *names[] = {
      "EV_COMMS_NO_EVENT",
      "EV_COMMS_PKT_RXD",
      "EV_COMMS_BOARD_TIMEDOUT",
      "EV_COMMS_BD_FIRST_PACKET",
      "EV_COMMS_Length",
  };

  bool outOfRange(uint8_t ev)
  {
    return ev >= Event::EV_COMMS_Length;
  }

  const char *getName(uint8_t ev)
  {
    if (!outOfRange(ev))
      return names[ev];
    return "WARNING: OUT OF RANGE";
  }

  void printEvent(uint8_t ev)
  {
    Serial.printf("--> CommsEvent: %s\n", outOfRange(ev)
                                              ? "OUT OF RANGE"
                                              : names[ev]);
  }
} // namespace CommsState
//------------------------------------------

/* prototypes */
// void sendToCommsEventStateQueue(CommsState::Event ev, TickType_t ticks = 5);
CommsState::Event readFromCommsStateEventQueue(TickType_t ticks = 5);
bool boardVersionCompatible(float version);
//------------------------------------------

bool comms_session_started = false;
bool comms_state_connected = false;
bool commsStateTask_initialised = false;

bool skipOnEnter = false;

CommsState::Event lastCommsEvent = CommsState::EV_COMMS_NO_EVENT;

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
      Serial.printf("stateCommsConnected\n");
      //       if (commsFsm->lastEvent() == CommsState::EV_COMMS_BD_FIRST_PACKET)
      //       {
      //         stats.boardResets++;
      //         displayChangeQueueManager->send(DISP_EV_UPDATE);
      //       }
      // #ifdef PRINT_COMMS_STATE
      //       commsFsm->print("stateCommsConnected");
      // #endif

      //       comms_session_started = true;
      //       comms_state_connected = true;

      //       displayChangeQueueManager->send(DISP_EV_CONNECTED);
      //       displayChangeQueueManager->send(DISP_EV_UPDATE);

      //       if (stats.needToAckResets())
      //       {
      //         displayChangeQueueManager->send(DISP_EV_SW_RESET);
      //       }

      //       // check board version is compatible
      //       bool boardCompatible = boardVersionCompatible(board.packet.version);
      //       if (!boardCompatible)
      //       {
      //         displayChangeQueueManager->send(DISP_EV_VERSION_DOESNT_MATCH);
      //       }
    },
    [] {
      if (sinceReported > 100)
      {
        sinceReported = 0;
        Serial.printf("reporting\n");
      }
    },
    NULL);

State stateCommsDisconnected(
    [] {
      if (commsFsm->lastEvent() == CommsState::EV_COMMS_BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DISP_EV_UPDATE);
      }

#ifdef PRINT_COMMS_STATE
      commsFsm->print("stateCommsDisconnected");
      // DEBUGMVAL("timed out", board.sinceLastPacket);
      DEBUGVAL(board.sinceLastPacket);
#endif

      comms_state_connected = false;
      displayChangeQueueManager->send(DISP_EV_DISCONNECTED);
    },
    NULL, NULL);
//-----------------------------------------------------

void addCommsStateTransitions()
{
  // CommsState::EV_COMMS_PKT_RXD
  commsFsm->add_transition(&stateCommsSearching, &stateCommsConnected, CommsState::EV_COMMS_PKT_RXD, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsConnected, CommsState::EV_COMMS_PKT_RXD, NULL);

  // CommsState::EV_COMMS_BOARD_TIMEDOUT
  commsFsm->add_transition(&stateCommsConnected, &stateCommsDisconnected, CommsState::EV_COMMS_BOARD_TIMEDOUT, NULL);

  // CommsState::EV_COMMS_BD_RESET
  commsFsm->add_transition(&stateCommsConnected, &stateCommsConnected, CommsState::EV_COMMS_BD_FIRST_PACKET, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsDisconnected, CommsState::EV_COMMS_BD_FIRST_PACKET, NULL);
}
//-----------------------------------------------------

void triggerCommsEvent(CommsState::Event ev)
{
#ifdef PRINT_COMMS_STATE_EVENT
  switch (ev)
  {
#ifdef SUPPRESS_EV_COMMS_PKT_RXD
  case CommsState::EV_COMMS_PKT_RXD:
    break;
#endif
  default:
    CommsState::printEvent(ev);
  }
#endif
  // lastCommsEvent = ev;
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
    if (ev != NO_QUEUE_EVENT)
    {
      if (ev != CommsState::EV_COMMS_NO_EVENT && ev < CommsState::EV_COMMS_Length)
      {
        Serial.printf("comms event: %s\n", CommsState::names[ev]);
        triggerCommsEvent((CommsState::Event)ev);
      }
      else
      {
        Serial.printf("comms event (alt): %d\n", ev);
      }
    }
    commsFsm->run_machine();

    vTaskDelay(500);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

CommsState::Event readFromCommsStateEventQueue(TickType_t ticks)
{
  uint8_t e;
  if (xCommsStateEventQueue != NULL && xQueueReceive(xCommsStateEventQueue, &e, ticks) == pdPASS)
  {
    if ((CommsState::Event)e == CommsState::EV_COMMS_BD_FIRST_PACKET)
    {
    }

    return (CommsState::Event)e;
  }
  return CommsState::EV_COMMS_NO_EVENT;
}

//-----------------------------------------------------
// check version
bool boardVersionCompatible(float version)
{
  return version == VERSION_BOARD_COMPAT;
}
