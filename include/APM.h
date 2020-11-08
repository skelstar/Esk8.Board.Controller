#ifndef ARDUINO_H
#include <Arduino.h>
#endif

enum PointState
{
  ON,
  OFF
};

#define POINTS_ARRAY_SIZE 100

class APM
{
  struct Point
  {
    PointState state;
    unsigned long time;
    char *note;
  };

public:
  APM(char *name)
  {
    _name = name;
    _running = false;
    // clear array
    for (int i = 0; i < POINTS_ARRAY_SIZE; i++)
    {
      _points[i].time = 0;
      _points[i].state = PointState::OFF;
    }
  }

  void start(bool now)
  {
    if (now)
    {
      _running = true;
      pointIdx = 0;
      _startTime = micros();
      Serial.printf("Started at %lums\n", micros());
    }
  }

  void addPoint(PointState state, char *note)
  {
    if (_running)
    {
      if (pointIdx >= POINTS_ARRAY_SIZE)
      {
        Serial.printf("ERROR: point array is getting too big!\n");
        return;
      }
      if (_points[pointIdx - 1].state == state)
      {
        Serial.printf("WARNING: adding same stage as last element\n");
        return;
      }

      _points[pointIdx].state = state;
      _points[pointIdx].time = micros() - _startTime;
      _points[pointIdx].note = note;
      pointIdx++;
    }
  }

  void stop()
  {
    if (_running)
    {
      _running = false;
      Serial.printf("Stopped at %lums\n", millis());
      printReport();
    }
  }

  bool running()
  {
    return _running;
  }

  void printReport()
  {
    Serial.printf("---------------------------------------------\n");
    Serial.printf("% report\n\n", _name);
    for (int i = 0; i < pointIdx; i++)
    {
      Serial.printf("%lu, %d, %s\n", _points[i].time, (int)_points[i].state, _points[i].note);
    }
    Serial.printf("---------------------------------------------\n");
  }

private:
  const char *stateToString(PointState state)
  {
    switch (state)
    {
    case ON:
      return "ON";
    case OFF:
      return "OFF";
    }
    return "UNKNOWN state";
  }

  unsigned long _startTime, _stopTime;
  char *_name;
  bool _running = false;
  Point _points[POINTS_ARRAY_SIZE];
  uint8_t pointIdx;
};