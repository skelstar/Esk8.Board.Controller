
enum Options
{
  NONE,
  NUM_ACCEL_COUNTS,
  NUM_BRAKE_COUNTS,
} showOption;

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
    DEBUG("NUM_ACCEL_COUNTS selected");
    break;
  case Options::NUM_BRAKE_COUNTS:
    DEBUG("NUM_BRAKE_COUNTS selected");
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
