

bool idFromBoardExpected(long id) {
  // return lastIdFromBoard == NO_PACKET_RECEIVED_FROM_BOARD 
  //     || id == lastIdFromBoard + 1;
  return true;
}

void powerpins_init()
{
  // deadman
  pinMode(DEADMAN_GND_PIN, OUTPUT);
  digitalWrite(DEADMAN_GND_PIN, LOW);
  // encoder
  pinMode(ENCODER_PWR_PIN, OUTPUT);
  digitalWrite(ENCODER_PWR_PIN, HIGH);
  pinMode(ENCODER_GND_PIN, OUTPUT);
  digitalWrite(ENCODER_GND_PIN, LOW);
}