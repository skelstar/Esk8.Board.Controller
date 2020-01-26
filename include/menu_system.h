#ifndef Fsm
#include <Fsm.h>
#endif

void display_state_event(DispStateEvent ev);

State disp_state_stopped_screen(
    [] {
      DEBUG("...disp_state_stopped_screen");
      screen_with_stats();
    },
    NULL, NULL);

State disp_state_moving_screen(
    [] {
      DEBUG("...disp_state_moving_screen");
      screen_moving();
    },
    NULL, NULL);

State disp_state_menu_option_1(
    [] {
      DEBUG("...disp_state_menu_option_1");
      u8g2.clearBuffer();
      lcd_message("option 1", MC_DATUM);
      u8g2.sendBuffer();
    },
    NULL, NULL);

State disp_state_menu_option_2(
    [] {
      DEBUG("...disp_state_menu_option_2");
      u8g2.clearBuffer();
      lcd_message("option 2", MC_DATUM);
      u8g2.sendBuffer();
    },
    NULL, NULL);

State disp_state_menu_option_3(
    [] {
      DEBUG("...disp_state_menu_option_3");
      u8g2.clearBuffer();
      lcd_message("option 3", MC_DATUM);
      u8g2.sendBuffer();
    },
    NULL, NULL);

Fsm display_state(&disp_state_stopped_screen);

void add_disp_state_transitions()
{
  // main - stopped
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_menu_option_1, DISP_EV_BUTTON_CLICK, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_REFRESH, NULL);

  // moving
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_moving_screen, DISP_EV_MOVING, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_stopped_screen, DISP_EV_STOPPED, NULL);

  // option 1
  display_state.add_timed_transition(&disp_state_menu_option_1, &disp_state_stopped_screen, 2000, NULL);
  display_state.add_transition(&disp_state_menu_option_1, &disp_state_menu_option_2, DISP_EV_BUTTON_CLICK, NULL);

  // option 2
  display_state.add_timed_transition(&disp_state_menu_option_2, &disp_state_stopped_screen, 2000, NULL);
  display_state.add_transition(&disp_state_menu_option_2, &disp_state_menu_option_3, DISP_EV_BUTTON_CLICK, NULL);

  // option 3
  display_state.add_timed_transition(&disp_state_menu_option_3, &disp_state_stopped_screen, 2000, NULL);
  display_state.add_transition(&disp_state_menu_option_3, &disp_state_stopped_screen, DISP_EV_BUTTON_CLICK, NULL);
}

char *get_event_name(DispStateEvent ev)
{
  switch (ev)
  {
  case DISP_EV_NO_EVENT:
    return "DISP_EV_NO_EVENT";
  case DISP_EV_BUTTON_CLICK:
    return "DISP_EV_BUTTON_CLICK";
  case DISP_EV_REFRESH:
    return "DISP_EV_REFRESH";
  case DISP_EV_STOPPED:
    return "DISP_EV_STOPPED";
  case DISP_EV_MOVING:
    return "DISP_EV_MOVING";
  default:
    return "Unhandled event";
  }
}

void display_state_event(DispStateEvent ev)
{
#ifdef PRINT_DISP_STATE_EVENT
  DEBUGVAL(get_event_name(ev));
#endif
  display_state.trigger(ev);
}