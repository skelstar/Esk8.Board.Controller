#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <AS5600.h>

#include <Button2.h>

#include <FastMap.h>

Button2 button35(35);

FastMap upper, lower;

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

AMS_5600 ams5600;

int ang, lang = 0;

void button35_click(Button2 &btn)
{
  Serial.printf("button 35 click\n");
  ams5600.setStartPosition();
}

void button35_doubleClick(Button2 &btn)
{
  Serial.printf("button 35 doubleclick\n");
  // ams5600.setEndPosition();
}

/*******************************************************
/* Function: convertRawAngleToDegrees
/* In: angle data from AMS_5600::getRawAngle
/* Out: human readable degrees as float
/* Description: takes the raw angle and calculates
/* float value in degrees.
/*******************************************************/
float convertRawAngleToDegrees(word newAngle)
{
  /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */
  float retVal = newAngle * 0.087;
  ang = retVal;
  return retVal;
}
/*******************************************************
/* Function: convertScaledAngleToDegrees
/* In: angle data from AMS_5600::getScaledAngle
/* Out: human readable degrees as float
/* Description: takes the scaled angle and calculates
/* float value in degrees.
/*******************************************************/
float convertScaledAngleToDegrees(word newAngle)
{
  word startPos = ams5600.getStartPosition();
  word endPos = ams5600.getEndPosition();
  word maxAngle = ams5600.getMaxAngle();

  Serial.printf("\nstartPos: %d maxAngle: %d endPos: %d\n", startPos, maxAngle, endPos);

  float multipler = 0;

  /* max angle and end position are mutually exclusive*/
  if (maxAngle > 0)
  {
    if (startPos == 0)
      multipler = (maxAngle * 0.0878) / 4096;
    else /*startPos is set to something*/
      multipler = ((maxAngle * 0.0878) - (startPos * 0.0878)) / 4096;
  }
  else
  {
    if ((startPos == 0) && (endPos == 0))
      multipler = 0.0878;
    else if ((startPos > 0) && (endPos == 0))
      multipler = ((360 * 0.0878) - (startPos * 0.0878)) / 4096;
    else if ((startPos == 0) && (endPos > 0))
      multipler = (endPos * 0.0878) / 4096;
    else if ((startPos > 0) && (endPos > 0))
      multipler = ((endPos * 0.0878) - (startPos * 0.0878)) / 4096;
  }
  return (newAngle * multipler);
}

void i2cScanner()
{
  Serial.printf("\n\ni2c scanner\n\n");

  bool found = false;
  elapsedMillis since_looking;

  while (!found)
  {
    Serial.printf("scanning\n");
    for (int addr = 1; addr < 127; addr++)
    {
      Wire.beginTransmission(addr);
      byte error = Wire.endTransmission();

      if (error == 0)
      {
        Serial.printf("device found at 0x02%x\n", addr);
        found = true;
      }
      else if (error == 4)
      {
        Serial.printf("unknown error found at 0x02%x\n", addr);
      }
    }

    if (!found)
      delay(2000);
  }

  Serial.printf("Finished scanning for devices\n");
}

//---------------------------------------------------------

elapsedMillis since_read_angle;
bool outlier = false;
float last_angle = -1,
      mapped = 0,
      mapped_centre = 0,
      mapped_max = 0,
      mapped_min = 0,
      mapped_offset = 0;

void setup()
{
  // delay(1000); // for comms reasons
  Serial.begin(115200);
  Serial.printf("Mag Encoder Ready\n");

  button35.setPressedHandler(button35_click);
  button35.setDoubleClickHandler(button35_doubleClick);

  Wire.begin();

  // i2cScanner();

  if (ams5600.detectMagnet() == 0)
  {
    Serial.printf("searching....\n");
    while (1)
    {
      if (ams5600.detectMagnet() == 1)
      {
        Serial.printf("Current Magnitude: %d\n", ams5600.getMagnitude());
        break;
      }
      else
      {
        Serial.println("Can not detect magnet");
      }
      delay(1000);
    }
  }

  // ams5600.setStartPosition(ams5600.getRawAngle());
  // ams5600.setEndPosition(0);
  // ams5600.setMaxAngle();

#define SWEEP_ANGLE 30.0

  float current = convertRawAngleToDegrees(ams5600.getRawAngle());
  last_angle = current;

  mapped_min = current - SWEEP_ANGLE;
  mapped_max = current + SWEEP_ANGLE;
  mapped_centre = current;

  Serial.printf("\n\nmin: %0.1f  ", mapped_min);
  Serial.printf("centre: %0.1f  ", mapped_centre);
  Serial.printf("max: %0.1f  ", mapped_max);
  Serial.println();

  outlier = current < SWEEP_ANGLE || current > 360 - SWEEP_ANGLE;

  if (outlier)
  {
    float l_min_i, l_min_o, l_max_i, l_max_o;
    float u_min_i, u_min_o, u_max_i, u_max_o;

    l_min_i = mapped_min + 360.0;
    l_max_i = 360.0;
    l_min_o = 0;
    l_max_o = abs(mapped_min);
    lower.init(l_min_i, l_max_i, l_min_o, l_max_o);

    u_min_i = 0;
    u_max_i = mapped_max;
    u_min_o = abs(mapped_min);
    u_max_o = SWEEP_ANGLE * 2;
    upper.init(u_min_i, u_max_i, u_min_o, u_max_o);

    Serial.printf("outlier ");
    Serial.printf("current: %.1f ", current);
    Serial.printf("lower: %.1f->%.1f => %.1f->%.1f |  ", l_min_i, l_max_i, l_min_o, l_max_o);
    Serial.printf("upper: %.1f->%.1f => %.1f->%.1f\n", u_min_i, u_max_i, u_min_o, u_max_o);
  }
  else
  {
    upper.init(mapped_min, mapped_max, 0, SWEEP_ANGLE * 2);
    Serial.printf("normal current: %.1f upper: %.1f->%.1f \n", current, mapped_min, mapped_max);
  }

  delay(1000);
}

#include <stdlib.h>
#include <math.h>

float constrainAngleToSweep(float x)
{
  if (outlier)
  {
    if (x > mapped_min + 360)
      return lower.constrainedMap(x);
    else
      return upper.constrainedMap(x);
  }
  else
  {
    return upper.map(x);
  }
}

void loop()
{
  button35.loop();

  if (since_read_angle > 100)
  {
    since_read_angle = 0;
    float angle = convertRawAngleToDegrees(ams5600.getRawAngle());

    float delta = last_angle - angle;

    if (abs(delta) > 0.5)
    {
      Serial.printf("raw: %0.1fdeg   ", angle);
      // Serial.printf("last: %0.1fdeg ", last_angle);
      // Serial.printf("delta: %0.1fdeg ", delta);
      // Serial.printf("start: %0.1fdeg ", mapped_centre);
      // Serial.printf("scaled: %0.1fdeg ", convertScaledAngleToDegrees(ams5600.getScaledAngle()));
      Serial.printf("constrained: %0.1fdeg   ", constrainAngleToSweep(angle));
      Serial.printf("\n");
    }
    last_angle = angle;
  }

  vTaskDelay(10);
}