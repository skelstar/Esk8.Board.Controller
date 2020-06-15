#ifndef Fsm
#include <Fsm.h>
#endif

const char *eventToString(DispStateEvent ev);
//---------------------------------------------------------------

void send_to_display_event_queue(DispStateEvent ev)
{
  TickType_t ticks = 10;
#ifdef PRINT_DISP_STATE_EVENT
  // Serial.printf("-> SEND: %s\n", get_event_name((DispStateEvent)ev));
#endif
  uint8_t e = (uint8_t)ev;
  xQueueSendToBack(xDisplayChangeEventQueue, &e, ticks);
}
//---------------------------------------------------------------

DispStateEvent read_from_display_event_queue(TickType_t ticks = 5)
{
  uint8_t e;
  if (xDisplayChangeEventQueue != NULL && xQueueReceive(xDisplayChangeEventQueue, &e, ticks) == pdPASS)
  {
#ifdef PRINT_DISP_STATE_EVENT
    Serial.printf("<- %s\n", eventToString((DispStateEvent)e));
#endif
    return (DispStateEvent)e;
  }
  return DISP_EV_NO_EVENT;
}
//---------------------------------------------------------------
void clearDisplayEventQueue()
{
  while (read_from_display_event_queue() != DISP_EV_NO_EVENT)
  {
  }
}
//---------------------------------------------------------------

OptionValue *currentOption;
DispStateEvent lastDispEvent;

void searching_onEnter();
void disconnected_onEnter();
void stopped_onEnter();
void moving_onEnter();

// prototypes
void print_disp_state(const char *state_name, const char *event);
void print_disp_state(const char *state_name);

//---------------------------------------------------------------
State disp_state_searching(searching_onEnter, NULL, NULL);
State disp_state_disconnected(disconnected_onEnter, NULL, NULL);
State disp_state_stopped_screen(stopped_onEnter, NULL, NULL);
State disp_state_moving_screen(moving_onEnter, NULL, NULL);
//---------------------------------------------------------------
void initWidgetsDisplay()
{
  tft.fillScreen(TFT_BLUE);
  widgetRsts->reset();
  widgetFail->reset();
  widgetThrottle->reset();
  widgetMissed->reset();
  widgetUnsuccessful->reset();
  widgetVolts->reset();
}
//---------------------------------------------------------------

Fsm display_state(&disp_state_searching);

void add_disp_state_transitions()
{
  // DISP_EV_CONNECTED
  display_state.add_transition(&disp_state_searching, &disp_state_stopped_screen, DISP_EV_CONNECTED, initWidgetsDisplay);
  display_state.add_transition(&disp_state_disconnected, &disp_state_stopped_screen, DISP_EV_CONNECTED, initWidgetsDisplay);
  // DISP_EV_DISCONNECTED
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);
  // DISP_EV_UPDATE
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_UPDATE, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_UPDATE, NULL);
  // DISP_EV_MOVING
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_moving_screen, DISP_EV_MOVING, NULL);
  // DISP_EV_STOPPED
  display_state.add_transition(&disp_state_moving_screen, &disp_state_stopped_screen, DISP_EV_STOPPED, NULL);
}
//---------------------------------------------------------------

const char *eventToString(DispStateEvent ev)
{
  switch (ev)
  {
  case DISP_EV_NO_EVENT:
    return "DISP_EV_NO_EVENT";
  case DISP_EV_CONNECTED:
    return "DISP_EV_CONNECTED";
  case DISP_EV_DISCONNECTED:
    return "DISP_EV_DISCONNECTED";
  case DISP_EV_STOPPED:
    return "DISP_EV_STOPPED";
  case DISP_EV_MOVING:
    return "DISP_EV_MOVING";
  case DISP_EV_UPDATE:
    return "DISP_EV_UPDATE";
  default:
    char buff[30];
    sprintf(buff, "unhandled ev: %d", (uint8_t)ev);
    return buff;
  }
}
//---------------------------------------------------------------

void print_disp_state(const char *state_name, const char *event)
{
#ifdef PRINT_DISP_STATE
  Serial.printf("%s --> %s\n", state_name, event);
#endif
}
//---------------------------------------------------------------

void searching_onEnter()
{
  print_disp_state("...disp_state_searching");
  screen_searching();
}
//---------------------------------------------------------------

void disconnected_onEnter()
{
  print_disp_state("...disp_state_disconnected");
  screen_with_stats(/*connected*/ false);
}
//---------------------------------------------------------------

void stopped_onEnter()
{
  print_disp_state("...disp_state_stopped_screen", eventToString(lastDispEvent));
  screenWithWidgets(/*connected*/ true);
}
//---------------------------------------------------------------

void moving_onEnter()
{
  print_disp_state("...disp_state_moving_screen");
  screenWithWidgets(/*connected*/ true);
}
//---------------------------------------------------------------

void print_disp_state(const char *state_name)
{
#ifdef PRINT_DISP_STATE
  Serial.printf("%s\n", state_name);
#endif
}
//---------------------------------------------------------------
