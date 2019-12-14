
void powerpins_init()
{
  // encoder
  pinMode(ENCODER_PWR_PIN, OUTPUT);
  digitalWrite(ENCODER_PWR_PIN, HIGH);
  pinMode(ENCODER_GND_PIN, OUTPUT);
  digitalWrite(ENCODER_GND_PIN, LOW);
}

bool boardOnline() 
{
  return lastPacketId + 1 >= sendCounter;
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
    default:
      return "unhandle reason";
  }
}
