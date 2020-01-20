
void powerpins_init()
{
  // encoder
  // pinMode(ENCODER_PWR_PIN, OUTPUT);
  // digitalWrite(ENCODER_PWR_PIN, HIGH);
  // pinMode(ENCODER_GND_PIN, OUTPUT);
  // digitalWrite(ENCODER_GND_PIN, LOW);
}

uint8_t printDot(uint8_t num_dots)
{
  if (num_dots++ < 60)
  {
    Serial.printf(".");
    return num_dots;
  }
  Serial.printf(".\n");
  return 0;
}

uint8_t getBatteryPercentage(float voltage) {
  float voltsLeft = voltage - BATTERY_VOLTAGE_CUTOFF_END;
  float voltsAvail = BATTERY_VOLTAGE_FULL - BATTERY_VOLTAGE_CUTOFF_END;

  uint8_t percent = 0;
  if ( voltage > BATTERY_VOLTAGE_CUTOFF_END ) { 
    percent = (voltsLeft /  voltsAvail) * 100;
  }
  if (percent > 100) {
    percent = 100;
	}
  return percent;
}

char* reason_toString(ReasonType reason)
{
  switch (reason)
  {
    case BOARD_STOPPED:
      return "BOARD_STOPPED";
    case BOARD_MOVING:
      return "BOARD_MOVING";
    case FIRST_PACKET:
      return "FIRST_PACKET";
    case LAST_WILL:
      return "LAST_WILL";
    case REQUESTED:
      return "REQUESTED";
    case VESC_OFFLINE:
      return "VESC_OFFLINE";
    default:
      return "unhandle reason ";
  }
}

bool print_throttle_flag = false;

void print_throttle(uint8_t target)
{
  if (target != controller_packet.throttle || print_throttle_flag)
  {
    print_throttle_flag = true;
    Serial.printf("target: %d t: %d ", target, controller_packet.throttle);
    Serial.println();
    if (target == controller_packet.throttle) 
    {
      print_throttle_flag = false;
    }
  }
}

void print_build_status()
{
  Serial.printf("\n");
  Serial.printf("/********************************************************\n");

#ifdef RELEASE_BUILD

  Serial.printf("               RELEASE BUILD!! \n");

#else

#ifdef USE_TEST_VALUES
  Serial.printf("               WARNING: Using test values!            \n");
#endif
#ifdef PRINT_BOARD_FSM_EVENT
  Serial.printf("               WARNING: PRINT_BOARD_FSM_EVENT\n");
#endif
#ifdef PRINT_BOARD_FSM_STATE_NAME
  Serial.printf("               WARNING: PRINT_BOARD_FSM_STATE\n");
#endif
#ifdef PRINT_TRIGGER_VALUE
  Serial.printf("               WARNING: PRINT_TRIGGER_VALUE\n");
#endif
#ifdef PACKET_RECV_DEBUG_ENABLED
  Serial.printf("               WARNING: PACKET_RECV_DEBUG_ENABLED\n");
#endif
#ifdef DEBUG_PRINT_STATE_NAME_ENABLED
  Serial.printf("               WARNING: DEBUG_PRINT_STATE_NAME_ENABLED\n");
#endif
#endif
  Serial.printf("/********************************************************/\n");
  Serial.printf("\n");
}

uint8_t get_remote_battery_percent(uint16_t raw_battery)
{
  uint16_t numerator = raw_battery > REMOTE_BATTERY_EMPTY
   ? raw_battery < REMOTE_BATTERY_FULL 
    ? raw_battery - REMOTE_BATTERY_EMPTY
    : REMOTE_BATTERY_FULL - REMOTE_BATTERY_EMPTY
   : 1;

  uint16_t denominator = REMOTE_BATTERY_FULL - REMOTE_BATTERY_EMPTY;
  
  return (numerator / (denominator*1.0)) * 100;
}
