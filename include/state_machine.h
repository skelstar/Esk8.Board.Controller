
enum StateMachineEventEnum
{
  EV_BUTTON_CLICK,
  EV_BOARD_CONNECTED,
  EV_STARTED_MOVING,
  EV_STOPPED_MOVING,
  EV_RECV_PACKET,
  EV_BOARD_FIRST_CONNECT,
  EV_BOARD_TIMEOUT, // havne't heard from the board for a while (BOARD_COMMS_TIMEOUT)
  EV_BOARD_LAST_WILL,
  EV_READ_TRIGGER_MIN,
  EV_READ_TRIGGER_MAX,
  EV_FINISHED_TRIGGER_CALIBRATION,
} fsm_event;

enum StateId
{
  STATE_CONNECTING,
  STATE_DISCONNECTED,
  STATE_NOT_MOVING,
  STATE_MOVING,
  STATE_WAITING_FOR_UPDATE,
  STATE_BOARD_TIMEDOUT,
  STATE_SHOW_BATTERY,
};

uint8_t get_prev_state();
void PRINT_STATE_NAME(const char *state_name);
void TRIGGER(StateMachineEventEnum x, char *s);

//-------------------------------
State state_connecting(
    STATE_CONNECTING,
    [] {
      PRINT_STATE_NAME("state_connecting --------");
      screen_show_connecting();
    },
    NULL,
    NULL);
//-------------------------------
State state_disconnected(
    STATE_DISCONNECTED,
    [] {
      PRINT_STATE_NAME("state_disconnected --------");
      char buffx[12];
      sprintf(buffx, "bd rsts: %d", board_first_packet_count);

      u8g2.clearBuffer();
      lcd_message(/*line#*/ 1, "Disconnected");
      lcd_message(/*line#*/ 3, &buffx[0]);
      u8g2.sendBuffer();
    },
    NULL,
    NULL);

//-------------------------------
State state_not_moving(
    STATE_NOT_MOVING,
    [] {
      PRINT_STATE_NAME("state_not_moving --------");
      screen_with_stats(trigger_fsm.get_current_state()->id, /*moving*/false);
      DEBUGVAL(board_packet.batteryVoltage);
    },
    [] {
      if (trigger_updated || stats.changed())
      {
        trigger_updated = false;
        screen_with_stats(trigger_fsm.get_current_state()->id, /*moving*/false);
      }
    },
    NULL);
//-------------------------------
State state_moving(
    STATE_MOVING,
    [] {
      PRINT_STATE_NAME("state_moving --------");
      // lcd_message("Moving");
    },
    NULL,
    NULL);
//-------------------------------
State state_show_battery(
    STATE_SHOW_BATTERY,
    [] {
      PRINT_STATE_NAME("state_show_battery --------");
      drawBattery(getBatteryPercentage(board_packet.batteryVoltage), true);
      char buffx[5];
      sprintf(buffx, "%3d", remote_battery_percent);
      lcd_message(/*line*/ 3, buffx);
      u8g2.sendBuffer();
    },
    NULL,
    NULL);
//-------------------------------
void handle_last_will()
{
  PRINT_STATE_NAME("last will received");
}
//-------------------------------

elapsedMillis since_reading_trigger = 0;
State state_trigger_centre(
    [] {
      PRINT_STATE_NAME("Trigger centre");
      lcd_message("Trig Center");
      since_reading_trigger = 0;
    },
    [] {
      trigger_centre = get_trigger_raw();
      if (since_reading_trigger > 1000)
      {
        TRIGGER(EV_READ_TRIGGER_MIN, NULL);
      }
    },
    [] {
      DEBUGVAL(trigger_centre);
      trigger_calibrated = true;
    });

Fsm fsm(&state_trigger_centre);

void addFsmTransitions()
{
  // trigger calibration
  fsm.add_transition(&state_trigger_centre, &state_connecting, EV_READ_TRIGGER_MIN, NULL);

  // state_connecting ->
  fsm.add_transition(&state_connecting, &state_not_moving, EV_BOARD_CONNECTED, NULL);
  fsm.add_transition(&state_connecting, &state_not_moving, EV_STOPPED_MOVING, NULL);
  fsm.add_transition(&state_connecting, &state_moving, EV_STARTED_MOVING, NULL);

  // -> state_disconnected
  fsm.add_transition(&state_not_moving, &state_disconnected, EV_BOARD_TIMEOUT, NULL);
  fsm.add_transition(&state_moving, &state_disconnected, EV_BOARD_TIMEOUT, NULL);

  // state_disconnected ->
  fsm.add_transition(&state_disconnected, &state_not_moving, EV_BOARD_CONNECTED, NULL);
  fsm.add_transition(&state_disconnected, &state_moving, EV_STARTED_MOVING, NULL);
  fsm.add_transition(&state_disconnected, &state_not_moving, EV_STOPPED_MOVING, NULL);

  // stopped -> moving -> stopped
  fsm.add_transition(&state_not_moving, &state_moving, EV_STARTED_MOVING, NULL);
  fsm.add_transition(&state_moving, &state_not_moving, EV_STOPPED_MOVING, NULL);

  // last will
  // fsm.add_transition(&state_main_screen, &state_main_screen, EV_BOARD_LAST_WILL, handle_last_will);

  // button 0 pressed
  fsm.add_transition(&state_not_moving, &state_show_battery, EV_BUTTON_CLICK, NULL);
  fsm.add_timed_transition(&state_show_battery, &state_not_moving, 1500, NULL);
}
/* ---------------------------------------------- */
uint8_t get_prev_state()
{
  return fsm.get_from_state();
}

void PRINT_STATE_NAME(const char *state_name)
{
#ifdef DEBUG_PRINT_STATE_NAME_ENABLED
  DEBUG(state_name);
#endif
}

void TRIGGER(StateMachineEventEnum x, char *s)
{
  if (s != NULL)
  {
#ifdef FSM_TRIGGER_DEBUG_ENABLED
    Serial.printf("EVENT: %s\n", s);
#endif
  }
  fsm.trigger(x);
}

/* ---------------------------------------------- */
/* ---------------------------------------------- */
/* ---------------------------------------------- */
