#ifndef Fsm
#include <Fsm.h>
#endif

enum DispStateEvent
{
  DISP_EV_BUTTON_CLICK,
};

void display_state_event(DispStateEvent ev);

State disp_state_main_screen(
    [] {
      DEBUG("...disp_state_main_screen");
      screen_with_stats();
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

Fsm display_state(&disp_state_main_screen);

void add_disp_state_transitions()
{
  display_state.add_transition(&disp_state_main_screen, &disp_state_menu_option_1, DISP_EV_BUTTON_CLICK, NULL);
  
  display_state.add_timed_transition(&disp_state_menu_option_1, &disp_state_main_screen, 2000, NULL);
  display_state.add_transition(&disp_state_menu_option_1, &disp_state_menu_option_2, DISP_EV_BUTTON_CLICK, NULL);
  
  display_state.add_timed_transition(&disp_state_menu_option_2, &disp_state_main_screen, 2000, NULL);
  display_state.add_transition(&disp_state_menu_option_2, &disp_state_menu_option_3, DISP_EV_BUTTON_CLICK, NULL);
  
  display_state.add_timed_transition(&disp_state_menu_option_3, &disp_state_main_screen, 2000, NULL);
  display_state.add_transition(&disp_state_menu_option_3, &disp_state_main_screen, DISP_EV_BUTTON_CLICK, NULL);
}

void display_state_event(DispStateEvent ev)
{
  display_state.trigger(ev);
}