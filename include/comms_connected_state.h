#ifndef Fsm
#include <Fsm.h>
#endif

enum CommsStateEvent
{
  EV_COMMS_NO_EVENT,
  EV_COMMS_PKT_RXD,
  EV_COMMS_BOARD_TIMEDOUT,
  EV_COMMS_BD_RESET,
};

enum CommsStateId
{
  ST_COMMS_SEARCHING,
  ST_COMMS_CONNECTED,
  ST_COMMS_DISCONNECTED
} currentCommsState;

//------------------------------------------

/* prototypes */
void sendToCommsEventStateQueue(CommsStateEvent ev, TickType_t ticks = 5);
CommsStateEvent readFromCommsStateEventQueue(TickType_t ticks = 5);
char *commsEventToString(CommsStateEvent ev);
void printStateName(const char *state_name);
void printStateName(const char *state_name, const char *event);
//------------------------------------------

bool comms_session_started = false;
bool comms_state_connected = false;
bool commsStateTask_initialised = false;

bool skipOnEnter = false;

CommsStateEvent lastCommsEvent = EV_COMMS_NO_EVENT;
//------------------------------------------

State stateCommsSearching([] {
  printStateName("stateCommsSearching");
  currentCommsState = ST_COMMS_SEARCHING;
},
                          NULL, NULL);

State stateCommsConnected([] {
  if (false == skipOnEnter)
  {
    printStateName("stateCommsConnected", commsEventToString(lastCommsEvent));
    comms_session_started = true;
    comms_state_connected = true;
    send_to_display_event_queue(DISP_EV_CONNECTED);
    currentCommsState = ST_COMMS_CONNECTED;
  }
  skipOnEnter = false;
},
                          NULL, NULL);

State stateCommsDisconnected([] {
  if (false == skipOnEnter)
  {
    printStateName("stateCommsDisconnected", commsEventToString(lastCommsEvent));
    comms_state_connected = false;
    send_to_display_event_queue(DISP_EV_DISCONNECTED);
    currentCommsState = ST_COMMS_DISCONNECTED;
  }
  skipOnEnter = false;
},
                             NULL, NULL);

void handleBdResetEvent()
{
  stats.boardResets++;
  send_to_display_event_queue(DISP_EV_BD_RSTS_CHANGED);
  skipOnEnter = true;
}

Fsm commsFsm(&stateCommsSearching);

//-----------------------------------------------------
void addCommsStateTransitions()
{
  commsFsm.add_transition(&stateCommsSearching, &stateCommsConnected, EV_COMMS_PKT_RXD, NULL);
  commsFsm.add_transition(&stateCommsConnected, &stateCommsDisconnected, EV_COMMS_BOARD_TIMEDOUT, NULL);
  commsFsm.add_transition(&stateCommsDisconnected, &stateCommsConnected, EV_COMMS_PKT_RXD, NULL);

  commsFsm.add_transition(&stateCommsDisconnected, &stateCommsDisconnected, EV_COMMS_BOARD_TIMEDOUT, NULL);

  // EV_COMMS_BD_RESET
  commsFsm.add_transition(&stateCommsConnected, &stateCommsConnected, EV_COMMS_BD_RESET, handleBdResetEvent);
  commsFsm.add_transition(&stateCommsDisconnected, &stateCommsDisconnected, EV_COMMS_BD_RESET, handleBdResetEvent);
}
//-----------------------------------------------------
char *commsEventToString(CommsStateEvent ev)
{
  switch (ev)
  {
  case EV_COMMS_PKT_RXD:
    return "EV_COMMS_PKT_RXD";
  case EV_COMMS_BOARD_TIMEDOUT:
    return "EV_COMMS_BOARD_TIMEDOUT";
  case EV_COMMS_BD_RESET:
    return "EV_COMMS_BD_RESET";
  case EV_COMMS_NO_EVENT:
    return "EV_COMMS_NO_EVENT";
  default:
    return "Handled event";
  }
}

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
  lastCommsEvent = ev;
  commsFsm.trigger(ev);
}

//-------------------------------------------------

void commsStateTask_0(void *pvParameters)
{

  Serial.printf("commsStateTask_0 running on core %d\n", xPortGetCoreID());

  commsStateTask_initialised = true;

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
    commsFsm.run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void sendToCommsEventStateQueue(CommsStateEvent ev, TickType_t ticks)
{
  uint8_t e = (uint8_t)ev;
  xQueueSendToBack(xCommsStateEventQueue, &e, ticks);
  if (ev == EV_COMMS_BD_RESET)
  {
    DEBUG(commsEventToString(ev));
  }
}
//------------------------------------------------------------

CommsStateEvent readFromCommsStateEventQueue(TickType_t ticks)
{
  uint8_t e;
  if (xCommsStateEventQueue != NULL && xQueueReceive(xCommsStateEventQueue, &e, ticks) == pdPASS)
  {
    if ((CommsStateEvent)e == EV_COMMS_BD_RESET)
    {
      DEBUG(commsEventToString((CommsStateEvent)e));
    }

    return (CommsStateEvent)e;
  }
  return CommsStateEvent::EV_COMMS_NO_EVENT;
}

void printStateName(const char *state_name)
{
#ifdef PRINT_COMMS_STATE
  DEBUG(state_name);
#endif
}

void printStateName(const char *state_name, const char *event)
{
#ifdef PRINT_COMMS_STATE
  Serial.printf("%s --> %s\n", state_name, event);
#endif
}
//------------------------------------------------------------
