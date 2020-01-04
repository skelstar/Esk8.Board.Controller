
// #define FSM_TRIGGER_DEBUG_ENABLED   1

enum StateMachineEventEnum
{
  EV_BUTTON_CLICK,
  EV_BOARD_CONNECTED,
  EV_STARTED_MOVING,
  EV_STOPPED_MOVING,
  EV_HELD_DOWN_WAIT,
  EV_NO_HELD_OPTION_SELECTED,
  EV_RECV_PACKET,
  EV_BOARD_FIRST_CONNECT,
  EV_PACKET_MISSED,
  EV_REQUESTED_UPDATE,
  EV_REQUESTED_RESPONSE,
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
void TRIGGER(StateMachineEventEnum x, char *s);


//-------------------------------
State state_connecting(
  STATE_CONNECTING,
  [] {
    DEBUG("state_connecting --------");
    lcdMessage("Connecting");
  },
  NULL,
  NULL);
//-------------------------------
State state_disconnected(
  STATE_DISCONNECTED,
  [] {
    DEBUG("state_disconnected --------");
    char buffx[12];
    sprintf(buffx, "missed: %.0f", nrf24.boardPacket.ampHours);

    u8g2.clearBuffer();
    lcdMessage(/*line#*/ 1, "Disconnected");
    lcdMessage(/*line#*/ 3, &buffx[0]);
    u8g2.sendBuffer();
 },
  NULL,
  NULL);

//-------------------------------
State state_not_moving(
  STATE_NOT_MOVING,
  [] {
    DEBUG("state_not_moving --------");
    char buffx[12];
    sprintf(buffx, "missed: %.0f", nrf24.boardPacket.ampHours);

    u8g2.clearBuffer();
    lcdMessage(/*line#*/ 1, "Stopped");
    lcdMessage(/*line#*/ 3, &buffx[0]);
    u8g2.sendBuffer();
  },
  NULL,
  NULL);
//-------------------------------
State state_moving(
  STATE_MOVING,
  [] {
    DEBUG("state_moving --------");
    lcdMessage("Moving");
  },
  NULL,
  NULL);
//-------------------------------
State state_show_battery(
  STATE_SHOW_BATTERY,
  [] {
    DEBUG("state_show_battery --------");
    drawBattery(getBatteryPercentage(nrf24.boardPacket.batteryVoltage), true);
  },
  NULL,
  NULL);
//-------------------------------
void handle_last_will() 
{
  DEBUG("last will received");
}
//-------------------------------

elapsedMillis since_reading_trigger = 0;
State state_trigger_centre(
  []{
    DEBUG("Trigger centre");
    lcdMessage("Trig Center");
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
  }
);
// State state_trigger_min(
//   []{
//     DEBUG("Trigger min");
//     lcdMessage("Trig Min");
//     since_reading_trigger = 0;
//   },
//   [] {
//     uint16_t min = get_trigger_raw();
//     if (min < trigger_min || min < trigger_centre)
//     {
//       trigger_min = min;
//     }
//     if (since_reading_trigger > 2000) 
//     {
//       TRIGGER(EV_READ_TRIGGER_MAX, NULL);
//     }
//   },
//   [] { DEBUGVAL(trigger_min); }
// );
// State state_trigger_max(
//   []{
//     DEBUG("Trigger max");
//     lcdMessage("Trig Max");
//     since_reading_trigger = 0;
//   },
//   [] {
//     uint16_t max = get_trigger_raw();
//     if (max > trigger_max)
//     {
//       trigger_max = max;
//     }
//     if (since_reading_trigger > 2000) 
//     {
//       TRIGGER(EV_FINISHED_TRIGGER_CALIBRATION, NULL);
//     }
//   },
//   [] { DEBUGVAL(trigger_max); }
// );

Fsm fsm(&state_trigger_centre);

void addFsmTransitions()
{
  // trigger calibration
  fsm.add_transition(&state_trigger_centre, &state_connecting, EV_READ_TRIGGER_MIN, NULL);
  // fsm.add_transition(&state_trigger_centre, &state_trigger_min, EV_READ_TRIGGER_MIN, NULL);
  // fsm.add_transition(&state_trigger_min, &state_trigger_max, EV_READ_TRIGGER_MAX, NULL);
  // fsm.add_transition(&state_trigger_max, &state_connecting, EV_FINISHED_TRIGGER_CALIBRATION, NULL);

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
