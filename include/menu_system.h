#ifndef Fsm
#include <Fsm.h>
#endif

enum DispStateEvent
{
  DISP_EV_NO_EVENT = 0,
  DISP_EV_BUTTON_CLICK,
  DISP_EV_REFRESH,
  DISP_EV_STOPPED,
  DISP_EV_MOVING,
};

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

void display_state_event(DispStateEvent ev)
{
  DEBUGVAL(ev);
  display_state.trigger(ev);
}