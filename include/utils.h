
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

void print_build_status(String chipId)
{
  Serial.printf("\n");
  Serial.printf("               Esk8.Board.Controller \n");
  Serial.printf("               Chip id: %s\n", chipId.c_str());

#ifdef RELEASE_BUILD
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               RELEASE BUILD!! \n");
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               %s \n", __TIME__);
  Serial.printf("               %s \n", __DATE__);
  Serial.printf("-----------------------------------------------\n");
#endif

  if (DEBUG_BUILD)
  {
    Serial.printf("-----------------------------------------------\n");
    Serial.printf("               DEBUG BUILD!! \n");
    Serial.printf("               BRANCH: %s \n", GIT_BRANCH_NAME);
    Serial.printf("-----------------------------------------------\n");
    Serial.printf("               %s \n", __TIME__);
    Serial.printf("               %s \n", __DATE__);
    Serial.printf("-----------------------------------------------\n");
  }
  Serial.printf("\n");
}
