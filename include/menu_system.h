#ifndef Fsm
#include <Fsm.h>
#endif

enum DispStateEvent
{
  DISP_EV_NO_EVENT = 0,
  DISP_EV_CONNECTED,
  DISP_EV_DISCONNECTED,
  DISP_EV_BUTTON_CLICK,
  DISP_EV_MENU_OPTION_SELECT,
  DISP_EV_REFRESH,
  DISP_EV_STOPPED,
  DISP_EV_MOVING,
};

void send_to_display_event_queue(DispStateEvent ev, TickType_t ticks = 10)
{
  xQueueSendToFront(xDisplayChangeEventQueue, &ev, ticks);
}

DispStateEvent read_from_display_event_queue(TickType_t ticks = 5)
{
  DispStateEvent e;
  if (xDisplayChangeEventQueue != NULL && xQueueReceive(xDisplayChangeEventQueue, &e, ticks) == pdPASS)
  {
    return e;
  }
  return DISP_EV_NO_EVENT;
}

void print_disp_state(const char *state_name);
void display_state_event(DispStateEvent ev);

//---------------------------------------------------------------
State disp_state_searching(
    [] {
      print_disp_state("...disp_state_searching");
      screen_searching();
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_disconnected(
    [] {
      print_disp_state("...disp_state_disconnected");
      screen_disconnected();
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_stopped_screen(
    [] {
      print_disp_state("...disp_state_stopped_screen");
      screen_with_stats();
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_moving_screen(
    [] {
      print_disp_state("...disp_state_moving_screen");
      // screen_moving();
      screen_with_stats();
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_menu_option_throttle_mode(
    [] {
      print_disp_state("...disp_state_menu_option_throttle_mode");
      lcd_message("throttle:", LINE_1, ALIGNED_LEFT);
      // lcd_message("deadman?", LINE_2, ALIGNED_LEFT);
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_menu_option_throttle_mode_selected(
    [] {
      print_disp_state("...disp_state_menu_option_throttle_mode_selected");
      // tft.fillScreen(TFT_BLUE);

      // switch (throttle.throttle_mode)
      // {
      // case THROTTLE_MODE_DEADMAN:
      //   throttle.throttle_mode = THROTTLE_MODE_PUSH_TO_START;
      //   lcd_message("push-to-start", LINE_2, ALIGNED_CENTRE);
      //   break;
      // case THROTTLE_MODE_PUSH_TO_START:
      //   throttle.throttle_mode = THROTTLE_MODE_DEADMAN;
      //   lcd_message("deadman", LINE_2, ALIGNED_CENTRE);
      //   break;
      // }
      // lcd_message("selected!", LINE_2, ALIGNED_CENTRE);
      // //u8g2.sendBuffer();
    },
    NULL, NULL);
//---------------------------------------------------------------

Fsm display_state(&disp_state_searching);

void add_disp_state_transitions()
{
  // searcing
  display_state.add_transition(&disp_state_searching, &disp_state_stopped_screen, DISP_EV_CONNECTED, NULL);
  // disconnected
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);
  // main - stopped
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_menu_option_throttle_mode, DISP_EV_BUTTON_CLICK, NULL);

  // moving
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_moving_screen, DISP_EV_MOVING, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_stopped_screen, DISP_EV_STOPPED, NULL);

  // options
  const uint16_t OPTION_SCREEN_TIMEOUT = 2000;
  const uint16_t OPTION_SELECTED_TIMEOUT = 1000;
  // option 1
  display_state.add_timed_transition(&disp_state_menu_option_throttle_mode, &disp_state_stopped_screen, OPTION_SCREEN_TIMEOUT, NULL);
  display_state.add_transition(&disp_state_menu_option_throttle_mode, &disp_state_stopped_screen, DISP_EV_BUTTON_CLICK, NULL);
  display_state.add_transition(&disp_state_menu_option_throttle_mode, &disp_state_menu_option_throttle_mode_selected, DISP_EV_MENU_OPTION_SELECT, NULL);
  // option  1selected
  display_state.add_timed_transition(&disp_state_menu_option_throttle_mode_selected, &disp_state_stopped_screen, OPTION_SELECTED_TIMEOUT, NULL);
}

const char *get_event_name(DispStateEvent ev)
{
  switch (ev)
  {
  case DISP_EV_NO_EVENT:
    return "DISP_EV_NO_EVENT";
  case DISP_EV_CONNECTED:
    return "DISP_EV_CONNECTED";
  case DISP_EV_DISCONNECTED:
    return "DISP_EV_DISCONNECTED";
  case DISP_EV_BUTTON_CLICK:
    return "DISP_EV_BUTTON_CLICK";
  case DISP_EV_MENU_OPTION_SELECT:
    return "DISP_EV_MENU_OPTION_SELECT";
  case DISP_EV_REFRESH:
    return "DISP_EV_REFRESH";
  case DISP_EV_STOPPED:
    return "DISP_EV_STOPPED";
  case DISP_EV_MOVING:
    return "DISP_EV_MOVING";
  default:
    char buff[20];
    sprintf(buff, "unhandled ev: %d", (uint8_t)ev);
    return buff;
  }
}

void print_disp_state(const char *state_name)
{
#ifdef PRINT_DISP_STATE
  DEBUG(state_name);
#endif
}

elapsedMillis since_last_refresh_event;

void display_state_event(DispStateEvent ev)
{
#ifdef PRINT_DISP_STATE_EVENT
  DEBUGVAL(get_event_name(ev));
#endif

  display_state.trigger(ev);
}