
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
  const char *line = "-----------------------------------------------\n";
  const char *spaces = "    ";

  Serial.printf("\n");
  Serial.printf(line);
  Serial.printf("%s Esk8.Board.Controller \n", spaces);
  Serial.printf("%s Chip id: %s\n", spaces, chipId.c_str());
  Serial.printf("\n");

  if (RELEASE_BUILD)
    Serial.printf("%s RELEASE BUILD!! \n", spaces);
  if (DEBUG_BUILD)
    Serial.printf("%s DEBUG BUILD!! \n", spaces);

  Serial.printf("\n");
  Serial.printf("%s BRANCH: %s \n", spaces, GIT_BRANCH_NAME);
  Serial.printf("\n");
  Serial.printf("%s %s \n", spaces, __TIME__);
  Serial.printf("%s %s \n", spaces, __DATE__);
  Serial.printf(line);
  Serial.printf("\n");
}

#define BATTERY_VOLTAGE_FULL 4.2 * 11         // 46.2
#define BATTERY_VOLTAGE_CUTOFF_START 3.4 * 11 // 37.4
#define BATTERY_VOLTAGE_CUTOFF_END 3.1 * 11   // 34.1

uint8_t getBatteryPercentage(float voltage)
{
  float voltsLeft = voltage - BATTERY_VOLTAGE_CUTOFF_END;
  float voltsAvail = BATTERY_VOLTAGE_FULL - BATTERY_VOLTAGE_CUTOFF_END;

  uint8_t percent = 0;
  if (voltage > BATTERY_VOLTAGE_CUTOFF_END)
  {
    percent = (voltsLeft / voltsAvail) * 100;
  }
  if (percent > 100)
  {
    percent = 100;
  }
  return percent;
}

// void i2cScanner()
// {
//   Serial.printf("\n\ni2c scanner\n\n");

//   bool found = false;
//   elapsedMillis since_looking;

//   while (!found)
//   {
//     Serial.printf("scanning\n");
//     for (int addr = 1; addr < 127; addr++)
//     {
//       Wire.beginTransmission(addr);
//       byte error = Wire.endTransmission();

//       if (error == 0)
//       {
//         Serial.printf("device found at 0x02%x\n", addr);
//         found = true;
//       }
//       else if (error == 4)
//       {
//         Serial.printf("unknown error found at 0x02%x\n", addr);
//       }
//     }

//     if (!found)
//       delay(2000);
//   }

//   Serial.printf("Finished scanning for devices\n");
// }
