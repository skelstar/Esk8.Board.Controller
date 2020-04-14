
enum Options
{
  NONE,
  NUM_ACCEL_COUNTS,
  NUM_BRAKE_COUNTS,
  HEADLIGHT_MODE,
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
}
//---------------------------------------------------------------
void displayCurrentOption()
{
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
  case Options::HEADLIGHT_MODE:
    return "Headlight";
  }
}
//---------------------------------------------------------------
