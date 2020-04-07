
enum Options
{
  NONE,
  NUM_ACCEL_COUNTS,
  NUM_BRAKE_COUNTS,
} showOption;

int16_t optionVal = 0;

//---------------------------------------------------------------
void moveToNextMenuItem()
{
  showOption = (int)showOption == (int)NUM_BRAKE_COUNTS
                   ? NUM_ACCEL_COUNTS
                   : (Options)((int)showOption + 1);
}
//---------------------------------------------------------------
void storeOption(OptionValue *currentOption)
{
  switch (showOption)
  {
  case Options::NONE:
    break;
  case Options::NUM_ACCEL_COUNTS:
    configStore.putUInt(STORE_CONFIG_ACCEL_COUNTS, currentOption->get());
    throttle.setMax(currentOption->get());
    config.accelCounts = currentOption->get();
    break;
  case Options::NUM_BRAKE_COUNTS:
    configStore.putUInt(STORE_CONFIG_BRAKE_COUNTS, currentOption->get());
    throttle.setMin(currentOption->get());
    config.brakeCounts = currentOption->get();
    break;
  }
}
//---------------------------------------------------------------
void displayCurrentOption()
{
  char buff2[20];

  switch (showOption)
  {
  case Options::NONE:
    showOption = Options::NUM_ACCEL_COUNTS;
  case Options::NUM_ACCEL_COUNTS:
    DEBUG("NUM_ACCEL_COUNTS");
    tft.fillScreen(TFT_DARKGREEN);
    lcd_message("Accel counts", LINE_1, Aligned::ALIGNED_CENTRE);
    sprintf(buff2, "%d", config.accelCounts);
    lcd_message(buff2, LINE_2, Aligned::ALIGNED_CENTRE);
    optionVal = config.accelCounts;
    break;
  case Options::NUM_BRAKE_COUNTS:
    DEBUG("NUM_BRAKE_COUNTS");
    tft.fillScreen(TFT_RED);
    lcd_message("Brake counts", LINE_1, Aligned::ALIGNED_CENTRE);
    sprintf(buff2, "%d", config.brakeCounts);
    lcd_message(buff2, LINE_2, Aligned::ALIGNED_CENTRE);
    optionVal = config.brakeCounts;
    break;
  }
}
//---------------------------------------------------------------
char *getTitleForMenuOption(Options option)
{
  switch (option)
  {
  case Options::NONE:
    return "";
  case Options::NUM_ACCEL_COUNTS:
    return "Accel counts";
  case Options::NUM_BRAKE_COUNTS:
    return "Brake counts";
  }
}
//---------------------------------------------------------------
