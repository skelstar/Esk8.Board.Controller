#ifndef Fsm
#include <Fsm.h>
#endif

const char *get_event_name(DispStateEvent ev);
//---------------------------------------------------------------

void send_to_display_event_queue(DispStateEvent ev)
{
  TickType_t ticks = 10;
#ifdef PRINT_DISP_STATE_EVENT
  Serial.printf("-> SEND: %s\n", get_event_name((DispStateEvent)ev));
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
    Serial.printf("<- RX: %s\n", get_event_name((DispStateEvent)e));
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

// prototypes

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
      showOption = Options::NONE;
      if (throttle.getMode() != EncoderMode::THROTTLE)
      {
        throttle.setMode(EncoderMode::THROTTLE);
      }
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_moving_screen(
    [] {
      print_disp_state("...disp_state_moving_screen");
      screen_moving();
      // screen_with_stats();
    },
    NULL, NULL);
//---------------------------------------------------------------
State disp_state_options(
    [] {
      print_disp_state("...disp_state_options");
      display_task_showing_option_screen = true;

      clearDisplayEventQueue();

      if (showOption == Options::NONE)
      {
        showOption = NUM_ACCEL_COUNTS;
      }

      switch (showOption)
      {
      case Options::NUM_ACCEL_COUNTS:
        currentOption = new OptionValue(/*min*/ 0, /*max*/ 50, /*curr*/ config.accelCounts, /*step*/ 5);
        currentOption->setBgColour(TFT_DARKGREEN);
        screenShowOptionWithValue(getTitleForMenuOption(showOption), currentOption);
        break;
      case Options::NUM_BRAKE_COUNTS:
        currentOption = new OptionValue(/*min*/ 0, /*max*/ 50, /*curr*/ config.brakeCounts, /*step*/ 5);
        currentOption->setBgColour(TFT_RED);
        screenShowOptionWithValue(getTitleForMenuOption(showOption), currentOption);
        break;
      default:
        Serial.printf("No showOption or unhandled: %d", showOption);
        break;
      }

      if (throttle.getMode() != EncoderMode::THROTTLE)
      {
        throttle.setMode(EncoderMode::MENU_OPTION);
      }
    },
    NULL,
    [] {
      display_task_showing_option_screen = false;
    });
//---------------------------------------------------------------
State disp_state_options_changed_up(
    [] {
      print_disp_state("...disp_state_options_changed_up");
      display_task_showing_option_screen = true;
      currentOption->up();
      screenShowOptionWithValue(getTitleForMenuOption(showOption), currentOption);
    },
    NULL,
    [] {
      display_task_showing_option_screen = false;
    });
//---------------------------------------------------------------
State disp_state_options_changed_dn(
    [] {
      print_disp_state("...disp_state_options_changed_dn");
      display_task_showing_option_screen = true;
      currentOption->dn();
      screenShowOptionWithValue(getTitleForMenuOption(showOption), currentOption);
    },
    NULL,
    [] {
      display_task_showing_option_screen = false;
    });
//---------------------------------------------------------------
State disp_state_option_selected(
    [] {
      print_disp_state("...disp_state_option_selected");
      screenShowOptionValueSelected();
      storeOption(currentOption);
    },
    NULL,
    [] {
      throttle.setMode(EncoderMode::THROTTLE);
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
  // main - stopped
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_options, DISP_EV_MENU_BUTTON_CLICKED, NULL);
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_stopped_screen, DISP_EV_UPDATE, NULL);
  // moving
  display_state.add_transition(&disp_state_stopped_screen, &disp_state_moving_screen, DISP_EV_MOVING, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_REFRESH, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_moving_screen, DISP_EV_UPDATE, NULL);
  display_state.add_transition(&disp_state_moving_screen, &disp_state_stopped_screen, DISP_EV_STOPPED, NULL);

  // options
  const uint16_t OPTION_SCREEN_TIMEOUT = 5000;
  const uint16_t OPTION_SELECTED_TIMEOUT = 1000;

  // option 1
  display_state.add_timed_transition(&disp_state_options, &disp_state_stopped_screen, OPTION_SCREEN_TIMEOUT, NULL);
  display_state.add_transition(&disp_state_options, &disp_state_options, DISP_EV_MENU_BUTTON_CLICKED, moveToNextMenuItem);
  display_state.add_transition(&disp_state_options, &disp_state_option_selected, DISP_EV_MENU_OPTION_SELECT, NULL);

  // disp_state_options_changed_up
  display_state.add_transition(&disp_state_options_changed_up, &disp_state_options, DISP_EV_MENU_BUTTON_CLICKED, moveToNextMenuItem);
  display_state.add_transition(&disp_state_options, &disp_state_options_changed_up, DISP_EV_ENCODER_UP, NULL);
  display_state.add_transition(&disp_state_options_changed_up, &disp_state_options_changed_up, DISP_EV_ENCODER_UP, NULL);
  display_state.add_transition(&disp_state_options_changed_dn, &disp_state_options_changed_up, DISP_EV_ENCODER_UP, NULL);
  display_state.add_transition(&disp_state_options_changed_up, &disp_state_option_selected, DISP_EV_OPTION_SELECT_VALUE, NULL);

  // disp_state_options_changed_dn
  display_state.add_transition(&disp_state_options_changed_dn, &disp_state_options, DISP_EV_MENU_BUTTON_CLICKED, moveToNextMenuItem);
  display_state.add_transition(&disp_state_options, &disp_state_options_changed_dn, DISP_EV_ENCODER_DN, NULL);
  display_state.add_transition(&disp_state_options_changed_dn, &disp_state_options_changed_dn, DISP_EV_ENCODER_DN, NULL);
  display_state.add_transition(&disp_state_options_changed_up, &disp_state_options_changed_dn, DISP_EV_ENCODER_DN, NULL);

  // disp_state_selected_option
  display_state.add_transition(&disp_state_options_changed_dn, &disp_state_option_selected, DISP_EV_OPTION_SELECT_VALUE, NULL);

  // option 1 selected
  display_state.add_timed_transition(&disp_state_option_selected, &disp_state_stopped_screen, OPTION_SELECTED_TIMEOUT, NULL);
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
  case DISP_EV_MENU_BUTTON_CLICKED:
    return "DISP_EV_MENU_BUTTON_CLICKED";
  case DISP_EV_MENU_OPTION_SELECT:
    return "DISP_EV_MENU_OPTION_SELECT";
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
