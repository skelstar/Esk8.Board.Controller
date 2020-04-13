#ifndef Fsm
#include <Fsm.h>
#endif

enum CommsStateEvent
{
  EV_COMMS_NO_EVENT,
  EV_COMMS_CONNECTED,
  EV_COMMS_DISCONNECTED,
  EV_COMMS_BD_RESET,
};

enum CommsStateId
{
  ST_COMMS_SEARCHING,
  ST_COMMS_CONNECTED,
  ST_COMMS_DISCONNECTED
};
//------------------------------------------

/* prototypes */
void send_to_comms_state_event_queue(CommsStateEvent ev, TickType_t ticks = 5);
CommsStateEvent read_from_comms_event_queue(TickType_t ticks = 5);
void print_state(const char *state_name);
//------------------------------------------

bool comms_session_started = false;
bool comms_state_connected = false;
bool commsStateTask_initialised = false;
//------------------------------------------

class CommsState
{
public:
  CommsState()
  {
  }

  void init()
  {
    _current = ST_COMMS_SEARCHING;
    // comms_session_started = true;
    print_state("...comms_state_searching");
  }

  CommsStateId get()
  {
    return _current;
  }

  void trigger(CommsStateEvent event)
  {
    switch (_current)
    {
    case ST_COMMS_SEARCHING:
      if (event == EV_COMMS_CONNECTED && comms_session_started)
      {
        // comms_session_started = true;
        comms_state_connected = true;
        _current = ST_COMMS_CONNECTED;
        print_state("...comms_state_connected");
        send_to_display_event_queue(DISP_EV_CONNECTED);
      }
      break;
    case ST_COMMS_CONNECTED:
      if (event == EV_COMMS_DISCONNECTED)
      {
        comms_state_connected = false;
        _current = ST_COMMS_DISCONNECTED;
        print_state("...comms_state_disconnected");
        send_to_display_event_queue(DISP_EV_DISCONNECTED);
      }
      else if (event == EV_COMMS_BD_RESET)
      {
        stats.boardResets++;
        send_to_display_event_queue(DISP_EV_UPDATE);
      }
      break;
    case ST_COMMS_DISCONNECTED:
      if (event == EV_COMMS_CONNECTED)
      {
        _current = ST_COMMS_CONNECTED;
        comms_state_connected = true;
        print_state("...comms_state_connected");
        send_to_display_event_queue(DISP_EV_CONNECTED);
      }
      else if (event == EV_COMMS_BD_RESET)
      {
        stats.boardResets++;
        send_to_display_event_queue(DISP_EV_UPDATE);
      }
      break;
    }
  }

private:
  CommsStateId _current;
} commsState;
//-------------------------------------------------

void commsStateTask_0(void *pvParameters)
{

  Serial.printf("commsStateTask_0 running on core %d\n", xPortGetCoreID());

  elapsedMillis since_checked_comms_state;

  commsStateTask_initialised = true;

  commsState.init();

  while (false == display_task_initialised)
  {
    vTaskDelay(1);
  }

  while (true)
  {
    if (since_checked_comms_state > 50)
    {
      since_checked_comms_state = 0;

      CommsStateEvent ev = read_from_comms_event_queue();
      if (ev != EV_COMMS_NO_EVENT)
      {
        commsState.trigger(ev);
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void send_to_comms_state_event_queue(CommsStateEvent ev, TickType_t ticks)
{
#ifdef PRINT_COMMS_STATE_EVENT
  if (ev == EV_COMMS_CONNECTED && commsState.get() != ST_COMMS_CONNECTED)
  {
    DEBUG("-> SEND: Connected");
  }
  else if (ev == EV_COMMS_DISCONNECTED && commsState.get() == ST_COMMS_CONNECTED)
  {
    DEBUG("-> SEND: Disconnected!");
  }
  else
  {
  }
#endif
  uint8_t e = (uint8_t)ev;
  xQueueSendToBack(xCommsStateEventQueue, &e, ticks);
}
//------------------------------------------------------------

CommsStateEvent read_from_comms_event_queue(TickType_t ticks)
{
  uint8_t e;
  if (xCommsStateEventQueue != NULL && xQueueReceive(xCommsStateEventQueue, &e, ticks) == pdPASS)
  {
    CommsStateEvent ev = (CommsStateEvent)e;
#ifdef PRINT_COMMS_STATE_EVENT
    if (ev == EV_COMMS_CONNECTED && !comms_state_connected)
    {
      DEBUG("-> SEND: Connected");
    }
    else if (ev == EV_COMMS_DISCONNECTED && comms_state_connected)
    {
      DEBUG("-> SEND: Disconnected!");
    }
#endif
    return ev;
  }
  return CommsStateEvent::EV_COMMS_NO_EVENT;
}

void print_state(const char *state_name)
{
#ifdef PRINT_COMMS_STATE
  DEBUG(state_name);
#endif
}
//------------------------------------------------------------
