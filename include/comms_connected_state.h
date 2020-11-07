#ifndef Fsm
#include <Fsm.h>
#endif

enum CommsStateEvent
{
  EV_COMMS_NO_EVENT,
  EV_COMMS_PKT_RXD,
  EV_COMMS_BOARD_TIMEDOUT,
  EV_COMMS_BD_FIRST_PACKET,
};

//------------------------------------------

/* prototypes */
void sendToCommsEventStateQueue(CommsStateEvent ev, TickType_t ticks = 5);
CommsStateEvent readFromCommsStateEventQueue(TickType_t ticks = 5);
const char *commsEventToString(CommsStateEvent ev);
const char *commsEventToString(int ev);
//------------------------------------------

bool comms_session_started = false;
bool comms_state_connected = false;
bool commsStateTask_initialised = false;

bool skipOnEnter = false;

CommsStateEvent lastCommsEvent = EV_COMMS_NO_EVENT;

Fsm *commsFsm;

/* prototypes */

//------------------------------------------
State stateCommsSearching([] {
#ifdef PRINT_COMMS_STATE
  commsFsm->print("stateCommsSearching", false);
#endif
});

State stateCommsConnected([] {
  if (commsFsm->lastEvent() == EV_COMMS_BD_FIRST_PACKET)
  {
    stats.boardResets++;
    send_to_display_event_queue(DISP_EV_UPDATE);
  }
#ifdef PRINT_COMMS_STATE
  commsFsm->print("stateCommsConnected");
#endif

  comms_session_started = true;
  comms_state_connected = true;

  send_to_display_event_queue(DISP_EV_CONNECTED);
  send_to_display_event_queue(DISP_EV_UPDATE);

  if (stats.needToAckResets())
  {
    send_to_display_event_queue(DISP_EV_SW_RESET);
  }
});

State stateCommsDisconnected(
    [] {
      if (commsFsm->lastEvent() == EV_COMMS_BD_FIRST_PACKET)
      {
        stats.boardResets++;
        send_to_display_event_queue(DISP_EV_UPDATE);
      }

#ifdef PRINT_COMMS_STATE
      commsFsm->print("stateCommsDisconnected");
      // DEBUGMVAL("timed out", board.sinceLastPacket);
      DEBUGVAL(board.sinceLastPacket);
#endif

      comms_state_connected = false;
      send_to_display_event_queue(DISP_EV_DISCONNECTED);
    },
    NULL, NULL);
//-----------------------------------------------------

void addCommsStateTransitions()
{
  // EV_COMMS_PKT_RXD
  commsFsm->add_transition(&stateCommsSearching, &stateCommsConnected, EV_COMMS_PKT_RXD, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsConnected, EV_COMMS_PKT_RXD, NULL);

  // EV_COMMS_BOARD_TIMEDOUT
  commsFsm->add_transition(&stateCommsConnected, &stateCommsDisconnected, EV_COMMS_BOARD_TIMEDOUT, NULL);

  // EV_COMMS_BD_RESET
  commsFsm->add_transition(&stateCommsConnected, &stateCommsConnected, EV_COMMS_BD_FIRST_PACKET, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsDisconnected, EV_COMMS_BD_FIRST_PACKET, NULL);
}
//-----------------------------------------------------

const char *commsEventToString(CommsStateEvent ev)
{
  switch (ev)
  {
  case EV_COMMS_PKT_RXD:
    return "EV_COMMS_PKT_RXD";
  case EV_COMMS_BOARD_TIMEDOUT:
    return "EV_COMMS_BOARD_TIMEDOUT";
  case EV_COMMS_BD_FIRST_PACKET:
    return "EV_COMMS_BD_FIRST_PACKET";
  case EV_COMMS_NO_EVENT:
    return "EV_COMMS_NO_EVENT";
  default:
    return "Handled event";
  }
}
//-----------------------------------------------------

const char *commsEventToString(int ev)
{
  return commsEventToString((CommsStateEvent)ev);
}
//-----------------------------------------------------

void triggerCommsEvent(CommsStateEvent ev)
{
#ifdef PRINT_COMMS_STATE_EVENT
  switch (ev)
  {
#ifdef SUPPRESS_EV_COMMS_PKT_RXD
  case EV_COMMS_PKT_RXD:
    break;
#endif
  default:
    Serial.printf("--> CommsEvent: %s\n", commsEventToString(ev));
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
  commsFsm->setGetEventName(commsEventToString);

  addCommsStateTransitions();

  while (false == display_task_initialised)
  {
    vTaskDelay(1);
  }

  while (true)
  {
    CommsStateEvent ev = readFromCommsStateEventQueue();
    if (ev != EV_COMMS_NO_EVENT)
    {
      triggerCommsEvent(ev);
    }
    commsFsm->run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void sendToCommsEventStateQueue(CommsStateEvent ev, TickType_t ticks)
{
  uint8_t e = (uint8_t)ev;
  xQueueSendToBack(xCommsStateEventQueue, &e, ticks);
  if (ev == EV_COMMS_BD_FIRST_PACKET)
  {
    // DEBUG(commsEventToString(ev));
  }
}
//------------------------------------------------------------

CommsStateEvent readFromCommsStateEventQueue(TickType_t ticks)
{
  uint8_t e;
  if (xCommsStateEventQueue != NULL && xQueueReceive(xCommsStateEventQueue, &e, ticks) == pdPASS)
  {
    if ((CommsStateEvent)e == EV_COMMS_BD_FIRST_PACKET)
    {
    }

    return (CommsStateEvent)e;
  }
  return CommsStateEvent::EV_COMMS_NO_EVENT;
}

//-----------------------------------------------------
