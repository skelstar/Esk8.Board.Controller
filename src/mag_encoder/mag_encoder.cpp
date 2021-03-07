#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <AS5600.h>

#include <FastMap.h>

#include <utils.h>

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

AMS_5600 ams5600;

int ang, lang = 0;
//---------------------------------------------------------

// #include <MagThrottle.h>
#include <NintendoButtons.h>

//---------------------------------------------------------

class Thrtl
{
private:
  int16_t _throttle = 127;
  float _centre = 0.0;
  float _min = 0.0,
        _max = 0.0,
        _last = 0.0,
        _sweep = 0.0;
  FastMap _map;

public:
  void init(float sweep)
  {
    _sweep = sweep;
    centre();
    _map.init(0, 255, 0, 10 + 1 + 10);
  }

  float convertRawAngleToDegrees(word angle)
  {
    return angle * 0.087;
  }

  void centre()
  {
    _centre = convertRawAngleToDegrees(ams5600.getRawAngle());
    _throttle = 127;
    _last = _centre;
    // _min = _centre - _sweep;
    // if (_min < 0.0)
    //   _min = 360.0 - abs(_min);
    // if (_max > 360.0)
    //   _max =
  }

  bool debugged = false;

  void drawBar(int idx, char *buff, uint8_t len, uint8_t m)
  {
    // bottom
    for (int j = 0; j < len; j++)
    {
      Serial.printf("%d,", j);
      if (j < m)
        buff[idx] = ' ';
      else
        buff[idx] = '<';
      idx++;
    }
    buff[idx++] = '+';
    // upper
    for (int j = len + 1; j <= len + 1 + len; j++)
    {
      Serial.printf("%d,", j);
      if (j > m)
        buff[idx] = ' ';
      else
        buff[idx] = '>';
      idx++;
    }
    buff[idx] = '\0';
  }

  uint8_t printThrottle(float delta, uint16_t throttle, char *buff)
  {
    if (debugged == false)
    {
      // Serial.printf()
    }

    int i = 0;
    buff[i++] = delta > 0.0 ? '#' : ' ';
    buff[i++] = delta > 10.0 ? '#' : ' ';
    buff[i++] = delta > 20.0 ? '#' : ' ';
    buff[i++] = delta > 30.0 ? '#' : ' ';
    buff[i++] = delta > 40.0 ? '#' : ' ';
    buff[i++] = delta > 50.0 ? '!' : ' ';

    uint8_t m = _map.constrainedMap(throttle);

    drawBar(i, buff, /*len*/ 10, m);

    return m;
  }

  void update()
  {
    float deg = convertRawAngleToDegrees(ams5600.getRawAngle());
    float adj = deg;
    float delta = _last - deg;
    bool edge = false;
    if (abs(delta) > 180.0)
    {
      edge = true;
      if (_last > deg)
        adj += 360.0;
      else
        adj -= 360.0;
      delta = _last - adj;
    }
    _throttle += (delta / 360.0) * 255;
    if (_throttle > 255)
      _throttle = 255;
    else if (_throttle < 0)
      _throttle = 0;

    if (abs(delta) > 0.5)
    {
      char b[50];
      uint8_t debug = printThrottle(abs(delta), _throttle, b);
      Serial.printf("%s ", edge ? "EDGE" : "----");
      // Serial.printf("| deg(adj): %05.1f ", deg);
      // Serial.printf("| last: %05.1f", _last);
      Serial.printf("| delta: %05.1f", delta);
      Serial.printf("| throttle: %03d", _throttle);
      Serial.printf("| debug: %02d", debug);
      Serial.printf("  %s \n", b);
    }

    _last = deg;
  }

} thrtl;

//---------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.printf("\n\nMag Encoder Ready\n\n");

  Wire.begin();

  i2cScanner(); // in utils.h

  ClassicButtons::init();

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

  thrtl.init(30.0);

  // MagThrottle::init(
  //     /*sweep*/ 60,
  //     [](uint8_t throttle) {
  //       Serial.printf("   throttle: %d\n", throttle);
  //     });

  delay(1000);
}

uint8_t old_throttle;
elapsedMillis since_read_angle, since_read_classic;

void loop()
{
  if (since_read_angle > 100)
  {
    since_read_angle = 0;

    thrtl.update();

    // MagThrottle::loop();
  }

  if (since_read_classic > 100)
  {
    since_read_classic = 0;
    ClassicButtons::loop();
  }

  vTaskDelay(10);
}