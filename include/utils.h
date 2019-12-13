
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