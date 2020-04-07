
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
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               RELEASE BUILD!! \n");
  Serial.printf("-----------------------------------------------\n");
#endif
#ifdef DEBUG_BUILD
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               DEBUG BUILD!! \n");
  Serial.printf("-----------------------------------------------\n");
#endif
  Serial.printf("\n");
}

char *get_reset_reason_text(RESET_REASON reason)
{
  switch (reason)
  {
  case 1:
    return "POWERON_RESET"; /**<1, Vbat power on reset*/
  case 3:
    return "SW_RESET"; /**<3, Software reset digital core*/
  case 4:
    return "OWDT_RESET"; /**<4, Legacy watch dog reset digital core*/
  case 5:
    return "DEEPSLEEP_RESET"; /**<5, Deep Sleep reset digital core*/
  case 6:
    return "SDIO_RESET"; /**<6, Reset by SLC module, reset digital core*/
  case 7:
    return "TG0WDT_SYS_RESET"; /**<7, Timer Group0 Watch dog reset digital core*/
  case 8:
    return "TG1WDT_SYS_RESET"; /**<8, Timer Group1 Watch dog reset digital core*/
  case 9:
    return "RTCWDT_SYS_RESET"; /**<9, RTC Watch dog Reset digital core*/
  case 10:
    return "INTRUSION_RESET"; /**<10, Instrusion tested to reset CPU*/
  case 11:
    return "TGWDT_CPU_RESET"; /**<11, Time Group reset CPU*/
  case 12:
    return "SW_CPU_RESET"; /**<12, Software reset CPU*/
  case 13:
    return "RTCWDT_CPU_RESET"; /**<13, RTC Watch dog Reset CPU*/
  case 14:
    return "EXT_CPU_RESET"; /**<14, for APP CPU, reseted by PRO CPU*/
  case 15:
    return "RTCWDT_BROWN_OUT_RESET"; /**<15, Reset when the vdd voltage is not stable*/
  case 16:
    return "RTCWDT_RTC_RESET"; /**<16, RTC Watch dog reset digital core and rtc module*/
  default:
    return "NO_MEAN";
  }
}
