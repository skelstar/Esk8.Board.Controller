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

// prototypes

void print_disp_state(const char *state_name, const char *event);
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
      print_disp_state("...disp_state_stopped_screen", eventToString(lastDispEvent));
      screenWithWidgets(/*connected*/ true);
      showOption = Options::NONE;
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_moving_screen(
    [] {
      print_disp_state("...disp_state_moving_screen");
      screenWithWidgets(/*connected*/ true);
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_options(
    [] {
      print_disp_state("...disp_state_options");

      if (showOption == NONE)
      {
        showOption = HEADLIGHT_MODE;
      }

      switch (showOption)
      {
      case HEADLIGHT_MODE:
        currentOption = new OptionValue(/*min*/ 0, /*max*/ 1, /*curr*/ config.headlightMode, /*step*/ 1, /*wrap*/ true);
        currentOption->setBgColour(TFT_BLACK);
        screenShowOptionWithValue(getTitleForMenuOption(Options::HEADLIGHT_MODE), currentOption);
        display_task_showing_option_screen = true;
        break;
      }
    },
    NULL,
    [] {
      display_task_showing_option_screen = false;
    });
//---------------------------------------------------------------
State disp_state_options_cycled(
    [] {
      print_disp_state("...disp_state_options_cycled");
      display_task_showing_option_screen = true;
      currentOption->up();
      screenShowOptionWithValue(getTitleForMenuOption(Options::HEADLIGHT_MODE), currentOption);
    },
    NULL,
    [] {
      display_task_showing_option_screen = false;
    });
//---------------------------------------------------------------
// State disp_state_options_changed_dn(
//     [] {
//       print_disp_state("...disp_state_options_changed_dn");
//     },
//     NULL,
//     [] {
//       display_task_showing_option_screen = false;
//     });
//---------------------------------------------------------------
State disp_state_option_selected(
    [] {
      print_disp_state("...disp_state_option_selected");
      screenShowOptionValueSelected();
      if (showOption == Options::HEADLIGHT_MODE)
      {
        config.headlightMode = currentOption->get();
        DEBUG("Stored!");
      }
      else
      {
        DEBUG("Unhandled option");
      }
    },
    NULL,
    [] {
      display_task_showing_option_screen = false;
    });

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
  display_state.add_transition(&disp_state_disconnected, &disp_state_disconnected, DISP_EV_DISCONNECTED, NULL);

  // main - stopped
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_THROTTLE_CHANGED, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_options, DISP_EV_MENU_BUTTON_CLICKED, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_UPDATE, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_BD_RSTS_CHANGED, NULL);

  // moving
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_moving_screen, DISP_EV_MOVING, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_UPDATE, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_THROTTLE_CHANGED, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_stopped_screen, DISP_EV_STOPPED, NULL);

  // options
  const uint16_t OPTION_SCREEN_TIMEOUT = 5000;
  const uint16_t OPTION_SELECTED_TIMEOUT = 2000;

  // option
  display_state.add_timed_transition(&disp_state_options, &disp_state_stopped_screen, OPTION_SCREEN_TIMEOUT, NULL);
  display_state.add_transition(&disp_state_options, &disp_state_options_cycled, DISP_EV_MENU_BUTTON_DOUBLE_CLICKED, NULL);

  // disp_state_options_cycled
  display_state.add_transition(&disp_state_options_cycled, &disp_state_options_cycled, DISP_EV_MENU_BUTTON_CLICKED, NULL);
  display_state.add_transition(&disp_state_options_cycled, &disp_state_option_selected, DISP_EV_MENU_BUTTON_DOUBLE_CLICKED, NULL);

  // option 1 selected
  display_state.add_timed_transition(&disp_state_option_selected, &disp_state_stopped_screen, OPTION_SELECTED_TIMEOUT, NULL);
}

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
  case DISP_EV_MENU_BUTTON_CLICKED:
    return "DISP_EV_BTN_CLICKED";
  case DISP_EV_MENU_BUTTON_DOUBLE_CLICKED:
    return "DISP_EV_BTN_DBL_CLICKED";
  case DISP_EV_REFRESH:
    return "DISP_EV_REFRESH";
  case DISP_EV_STOPPED:
    return "DISP_EV_STOPPED";
  case DISP_EV_MOVING:
    return "DISP_EV_MOVING";
  case DISP_EV_ENCODER_UP:
    return "DISP_EV_ENCODER_UP";
  case DISP_EV_ENCODER_DN:
    return "DISP_EV_ENCODER_DN";
  case DISP_EV_OPTION_SELECT_VALUE:
    return "DISP_EV_OPTION_SELECT_VALUE";
  case DISP_EV_UPDATE:
    return "DISP_EV_UPDATE";
  case DISP_EV_THROTTLE_CHANGED:
    return "DISP_EV_THROTTLE_CHANGED";
  case DISP_EV_BD_RSTS_CHANGED:
    return "DISP_EV_BD_RSTS_CHANGED";
  default:
    char buff[30];
    sprintf(buff, "unhandled ev: %d", (uint8_t)ev);
    return buff;
  }
}

void print_disp_state(const char *state_name, const char *event)
{
#ifdef PRINT_DISP_STATE
  Serial.printf("%s --> %s\n", state_name, event);
#endif
}

void print_disp_state(const char *state_name)
{
#ifdef PRINT_DISP_STATE
  Serial.printf("%s\n", state_name);
#endif
}
