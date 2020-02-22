
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

const char *reason_toString(ReasonType reason)
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

void print_build_status()
{
  Serial.printf("\n");
#ifdef RELEASE_BUILD
  Serial.printf("-----------------------------------------------");
  Serial.printf("               RELEASE BUILD!! \n");
  Serial.printf("-----------------------------------------------");
#endif
#ifdef DEBUG_BUILD
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               DEBUG BUILD!! \n");
  Serial.printf("-----------------------------------------------\n");
#endif
  Serial.printf("\n");
}
