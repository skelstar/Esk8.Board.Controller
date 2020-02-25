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

const char *get_event_name(DispStateEvent ev);

void send_to_display_event_queue(DispStateEvent ev, TickType_t ticks = 10)
{
#ifdef PRINT_DISP_STATE_EVENT
  Serial.printf("-> SEND: %s\n", get_event_name((DispStateEvent)ev));
#endif
  uint8_t e = (uint8_t)ev;
  xQueueSendToBack(xDisplayChangeEventQueue, &e, ticks);
}

DispStateEvent read_from_display_event_queue(TickType_t ticks = 5)
{
  uint8_t e;
  if (xDisplayChangeEventQueue != NULL && xQueueReceive(xDisplayChangeEventQueue, &e, ticks) == pdPASS)
  {
#ifdef PRINT_DISP_STATE_EVENT
    Serial.printf("<- RX: %s\n", get_event_name((DispStateEvent)e));
#endif
    return (DispStateEvent)e;
  }
  return DISP_EV_NO_EVENT;
}

void print_disp_state(const char *state_name);

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
      screen_with_stats(/*connected*/ false);
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
State disp_state_menu_throttle(
    [] {
      print_disp_state("...disp_state_menu_throttle");
      tft.fillScreen(TFT_DARKGREEN);
      lcd_message("throttle..", LINE_1, ALIGNED_LEFT);
      switch (throttle.mode)
      {
      case ADVANCED:
        lcd_message("beginner?", LINE_2, ALIGNED_CENTRE);
        break;
      case BEGINNER:
        lcd_message("advanced", LINE_2, ALIGNED_CENTRE);
        break;
      }
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_menu_throttle_selected(
    [] {
      print_disp_state("...disp_state_menu_throttle_selected");

      switch (throttle.mode)
      {
      case ADVANCED:
        throttle.mode = BEGINNER;
        lcd_message("selected!", LINE_3, ALIGNED_CENTRE);
        break;
      case BEGINNER:
        throttle.mode = ADVANCED;
        lcd_message("selected!", LINE_3, ALIGNED_CENTRE);
        break;
      }
    },
    NULL, NULL);
//---------------------------------------------------------------

Fsm display_state(&disp_state_searching);

void add_disp_state_transitions()
{
  // searcing
  display_state.add_transition(&disp_state_searching, &disp_state_stopped_screen, DISP_EV_CONNECTED, NULL);
  display_state.add_transition(&disp_state_disconnected, &disp_state_stopped_screen, DISP_EV_CONNECTED, NULL);
  // disconnected
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);
  // main - stopped
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_menu_throttle, DISP_EV_BUTTON_CLICK, NULL);
  // moving
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_moving_screen, DISP_EV_MOVING, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_stopped_screen, DISP_EV_STOPPED, NULL);

  // options
  const uint16_t OPTION_SCREEN_TIMEOUT = 2000;
  const uint16_t OPTION_SELECTED_TIMEOUT = 1000;
  // option 1
  display_state.add_timed_transition(&disp_state_menu_throttle, &disp_state_stopped_screen, OPTION_SCREEN_TIMEOUT, NULL);
  display_state.add_transition(&disp_state_menu_throttle, &disp_state_stopped_screen, DISP_EV_BUTTON_CLICK, NULL);
  display_state.add_transition(&disp_state_menu_throttle, &disp_state_menu_throttle_selected, DISP_EV_MENU_OPTION_SELECT, NULL);
  // option  1selected
  display_state.add_timed_transition(&disp_state_menu_throttle_selected, &disp_state_stopped_screen, OPTION_SELECTED_TIMEOUT, NULL);
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
