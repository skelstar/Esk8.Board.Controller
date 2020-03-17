
enum Options
{
  NONE,
  NUM_ACCEL_COUNTS,
  NUM_BRAKE_COUNTS,
} showOption;

int16_t optionVal = 0;

//---------------------------------------------------------------
void moveToNextOption()
{
  showOption = (int)showOption == (int)NUM_BRAKE_COUNTS
                   ? NUM_ACCEL_COUNTS
                   : (Options)((int)showOption + 1);
}
//---------------------------------------------------------------
void selectOption()
{
  switch (showOption)
  {
  case Options::NUM_ACCEL_COUNTS:
    DEBUGVAL("NUM_ACCEL_COUNTS selected", optionVal);
    configStore.putUInt(STORE_CONFIG_ACCEL_COUNTS, optionVal);
    throttle.setMax(optionVal);
    break;
  case Options::NUM_BRAKE_COUNTS:
    DEBUGVAL("NUM_BRAKE_COUNTS selected", optionVal);
    configStore.putUInt(STORE_CONFIG_BRAKE_COUNTS, optionVal);
    throttle.setMin(optionVal);
    break;
  }
}
//---------------------------------------------------------------
void displayCurrentOption()
{
  switch (showOption)
  {
  case Options::NONE:
    showOption = Options::NUM_ACCEL_COUNTS;
  case Options::NUM_ACCEL_COUNTS:
    DEBUG("NUM_ACCEL_COUNTS");
    break;
  case Options::NUM_BRAKE_COUNTS:
    DEBUG("NUM_BRAKE_COUNTS");
    break;
  }
}
//---------------------------------------------------------------
void optionChange(bool up)
{
  uint8_t step = 1;
  switch (showOption)
  {
  case Options::NUM_ACCEL_COUNTS:
    step = 5;
    optionVal = up ? optionVal + step : optionVal - step;
    DEBUGVAL("NUM_ACCEL_COUNTS", optionVal);
    break;
  case Options::NUM_BRAKE_COUNTS:
    step = 5;
    const uint8_t step = 5;
    optionVal = up ? optionVal + step : optionVal - step;
    DEBUGVAL("NUM_BRAKE_COUNTS", optionVal);
    break;
  }
}
