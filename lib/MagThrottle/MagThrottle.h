#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

#include <SparkFun_Qwiic_Button.h>

namespace MagThrottle
{
  // https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

  AMS_5600 ams5600;

  const char *statePrintFormat = "[State]: %s\n";

  typedef void (*ThrottleChangedCallback)(uint8_t);
  typedef void (*ButtonEventCallback)(uint8_t);

  ThrottleChangedCallback _throttleChangedCb = nullptr;
  ButtonEventCallback _buttonEventCb = nullptr;

  enum ButtonEvent
  {
    CLICKED,
    RELEASED,
  };

  AMS_5600 *_ams = nullptr;

  FastMap _lower_map, _upper_map, _throttle_map;

  bool _wraps = false;
  float _sweep_angle = 0;
  float _last = -1,
        _currentAngle = 0,
        _prevAngle = 0,
        _centre = 0,
        _offset = 0,
        _upperLimit = 0,
        _lowerLimit = 0;

  //https://www.sparkfun.com/products/15932
  // int _buttoni2cAddr = 0x6f;
  QwiicButton button;

  uint8_t _throttle = 127,
          _old_throttle = 127,
          _currentZone = 0;

  void centre(bool print = false);
  void _addTransitions();
  float _convertRawAngleToDegrees(word newAngle);
  uint8_t _getZone(float raw, bool print = false);
  void _setMaps(bool print = false);
  void _print();
  float to360(float val);
  float _normaliseTo0toMax(float raw, bool print = false);
  uint8_t get(bool enabled = true);

  enum DialZone
  {
    LOWER_LIMIT,
    BRAKING,
    IDLE,
    ACCEL,
    UPPER_LIMIT,
  } _zone;

  const char *_getDialZone(DialZone state)
  {
    switch (state)
    {
    case LOWER_LIMIT:
      return "LOWER_LIMIT";
    case BRAKING:
      return "BRAKING";
    case IDLE:
      return "IDLE";
    case ACCEL:
      return "ACCEL";
    case UPPER_LIMIT:
      return "UPPER_LIMIT";
    }
    return "OUT OF RANGE (_getDialZone())";
  }

  enum MapParams
  {
    MIN_IN,
    MAX_IN,
    MIN_OUT,
    MAX_OUT,
  };

  float _l[4];
  float _u[4];

  enum FsmTrigger
  {
    TR_LOWER_LIMIT,
    TR_BRAKING,
    TR_IDLE,
    TR_ACCEL,
    TR_UPPER_LIMIT,
    TR_CENTRE,
  };

  const char *_getTrigger(FsmTrigger trigger)
  {
    switch (trigger)
    {
    case TR_LOWER_LIMIT:
      return "TR_LOWER_LIMIT";
    case TR_BRAKING:
      return "TR_BRAKING";
    case TR_IDLE:
      return "TR_IDLE";
    case TR_ACCEL:
      return "TR_ACCEL";
    case TR_UPPER_LIMIT:
      return "TR_UPPER_LIMIT";
    case TR_CENTRE:
      return "TR_CENTRE";
    }
    return "OUT OF RANGE (_getTrigger())";
  }

  enum StateList
  {
    ST_LOWER_LIMIT,
    ST_BRAKING,
    ST_IDLE,
    ST_ACCELERATING,
    ST_UPPER_LIMIT,
  } _currentState;

  const char *getStateList(StateList s)
  {
    switch (s)
    {
    case ST_LOWER_LIMIT:
      return "ST_LOWER_LIMIT";
    case ST_BRAKING:
      return "ST_BRAKING";
    case ST_IDLE:
      return "ST_IDLE";
    case ST_ACCELERATING:
      return "ST_ACCELERATING";
    case ST_UPPER_LIMIT:
      return "ST_UPPER_LIMIT";
    }
    return "OUT OF RANGE getStateList()";
  }

  void printState(StateList st)
  {
    if (PRINT_ZONE_CHANGE == 1)
      Serial.printf(statePrintFormat, getStateList(st));
  }

  void updateThrottle()
  {
    _old_throttle = _throttle;
    uint8_t t = get();

    if (_currentState != ST_LOWER_LIMIT && _currentState != UPPER_LIMIT)
      _throttle = t;

    if (_old_throttle != _throttle && _throttleChangedCb != nullptr)
      _throttleChangedCb(_throttle);
  }

  State stLowerLimit(
      [] {
        printState(ST_LOWER_LIMIT);
        _currentState = ST_LOWER_LIMIT;
        _throttle = 0;
        if (_throttleChangedCb != nullptr)
          _throttleChangedCb(_throttle);
      },
      updateThrottle,
      NULL);

  State stBraking(
      [] {
        printState(ST_BRAKING);
        _currentState = ST_BRAKING;
      },
      updateThrottle,
      NULL);

  State stIdle(
      [] {
        printState(ST_IDLE);
        _currentState = ST_IDLE;
        _throttle = 127;
        if (_throttleChangedCb != nullptr)
          _throttleChangedCb(_throttle);
      },
      updateThrottle,
      NULL);

  State stAccelerating(
      [] {
        printState(ST_ACCELERATING);
        _currentState = ST_ACCELERATING;
      },
      updateThrottle,
      NULL);

  State stUpperLimit(
      [] {
        printState(ST_UPPER_LIMIT);
        _currentState = ST_UPPER_LIMIT;
        _throttle = 255;
        if (_throttleChangedCb != nullptr)
          _throttleChangedCb(_throttle);
      },
      updateThrottle,
      NULL);

  void resetCentre()
  {
    centre(/*print*/ true);
  }

  Fsm fsm(&stIdle);

  //------------------------------------------------------
  void init(float sweep_angle, ThrottleChangedCallback throttleChangedCb)
  {
    _sweep_angle = sweep_angle;
    _throttleChangedCb = throttleChangedCb;

    _throttle_map.init(0, sweep_angle * 2, 0, 255);

    if (button.begin() == false)
    {
      Serial.printf("ERROR: Could not find button!\n");
    }

    centre(/*print*/ true);

    _addTransitions();
  }

  //------------------------------------------------------

  int _buttonState = 0;
  elapsedMillis sincePressedButton;

  void loop()
  {
    fsm.run_machine();

    int old_state = _buttonState;
    _buttonState = button.isPressed();

    if (old_state != _buttonState)
    {
      // Serial.printf("button state: %d\n", _buttonState);
      if (_buttonState == 1)
        sincePressedButton = 0;
      if (_buttonState == 0) // released
      {
        if (sincePressedButton < 500)
        {
          centre(/*print*/ true);
          if (_buttonEventCb != nullptr)
            _buttonEventCb(ButtonEvent::CLICKED);
        }
        else if (_buttonEventCb != nullptr)
          _buttonEventCb(ButtonEvent::RELEASED);
        centre(/*print*/ true);
      }
    }

    old_state = _buttonState;
  }

  FsmTrigger toTrigger(uint8_t zone)
  {
    switch (zone)
    {
    case LOWER_LIMIT:
      return TR_LOWER_LIMIT;
    case BRAKING:
      return TR_BRAKING;
    case IDLE:
      return TR_IDLE;
    case ACCEL:
      return TR_ACCEL;
    case UPPER_LIMIT:
      return TR_UPPER_LIMIT;
    }
    Serial.printf("WARNING: OUT OF RANGE toTrigger(DialZone) \n");
    return TR_IDLE;
  }

  uint8_t get(bool enabled)
  {
    _prevAngle = _currentAngle;
    _currentAngle = _convertRawAngleToDegrees(ams5600.getRawAngle());

    float _normalised = _normaliseTo0toMax(_currentAngle); // now between 0 - middle - 60 degrees

    _currentZone = _getZone(/*normalised*/ _normalised);
    fsm.trigger(toTrigger(_currentZone));

    if (abs(_prevAngle - _currentAngle) > 0.5)
    {
      // Serial.printf("_current: %.1f  |  _normalised: %.1f  |  centre: %.1f \n", _currentAngle, _normalised, _centre);
    }

    return _throttle_map.constrainedMap(_normalised);
  }

  void _addTransitions()
  {
    // stLowerLimit
    fsm.add_transition(&stLowerLimit, &stBraking, TR_BRAKING, NULL);

    // stBraking
    fsm.add_transition(&stBraking, &stIdle, TR_IDLE, NULL);
    fsm.add_transition(&stBraking, &stLowerLimit, TR_LOWER_LIMIT, NULL);
    fsm.add_transition(&stBraking, &stAccelerating, TR_ACCEL, NULL);

    // stIdle
    fsm.add_transition(&stIdle, &stAccelerating, TR_ACCEL, NULL);
    fsm.add_transition(&stIdle, &stBraking, TR_BRAKING, NULL);

    // stAccelerating
    fsm.add_transition(&stAccelerating, &stUpperLimit, TR_UPPER_LIMIT, NULL);
    fsm.add_transition(&stAccelerating, &stIdle, TR_IDLE, NULL);
    fsm.add_transition(&stAccelerating, &stBraking, TR_BRAKING, NULL);

    // stUpperLimit
    fsm.add_transition(&stUpperLimit, &stAccelerating, TR_ACCEL, NULL);

    // TR_CENTRE
    fsm.add_transition(&stLowerLimit, &stIdle, TR_CENTRE, resetCentre);
    fsm.add_transition(&stBraking, &stIdle, TR_CENTRE, resetCentre);
    fsm.add_transition(&stIdle, &stIdle, TR_CENTRE, resetCentre);
    fsm.add_transition(&stAccelerating, &stIdle, TR_CENTRE, resetCentre);
    fsm.add_transition(&stUpperLimit, &stIdle, TR_CENTRE, resetCentre);
  }

  float _convertRawAngleToDegrees(word newAngle)
  {
    /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */
    float retVal = newAngle * 0.087;
    return retVal;
  }

  namespace
  {
    void _setBoundaries(float raw)
    {
      _centre = raw;
      _lowerLimit = to360(_centre - _sweep_angle);
      _upperLimit = to360(_centre + _sweep_angle);
      _last = raw;
      _wraps = to360(_lowerLimit) > _upperLimit;
      _offset = _sweep_angle - _centre;

      // Serial.printf("_setBoundaries: min: %.1f   _centre:%.1f  _offset: %.1f  max: %.1f \n", _lowerLimit, _centre, _offset, _upperLimit);
    }
  }

  float _normaliseTo0toMax(float raw, bool print)
  {
    if (print)
    {
      Serial.printf("normalisedAngle: raw: %.1f   offset: %.1f   ", raw, _offset);
      Serial.printf("mapped: %.1f ", to360(raw + _offset));
      Serial.println();
    }

    return to360(raw + _offset);
  }

  void centre(bool print)
  {
    _currentAngle = _convertRawAngleToDegrees(ams5600.getRawAngle());

    _setBoundaries(_currentAngle);

    if (print)
      Serial.printf("throttle: centred: %.1f\n", _centre);
  }

  void _print()
  {
    Serial.println("\n\n------------------------------------------------------");
    if (_wraps)
    {
      Serial.printf("setMaps(%.1f): **WRAPS**  |  ", _currentAngle);
      Serial.printf("min: %.1f - max: %.1f  |  ", _lowerLimit, _upperLimit);
      Serial.printf("lower: %.1f - %.1f maps to %.1f - %.1f  |  ", _l[MIN_IN], _l[MAX_IN], _l[MIN_OUT], _l[MAX_OUT]);
      Serial.printf("upper: %.1f - %.1f maps to %.1f - %.1f", _u[MIN_IN], _u[MAX_IN], _u[MIN_OUT], _u[MAX_OUT]);
    }
    else
    {
      Serial.printf("setMaps(%.1f): NORMAL  |  ", _currentAngle);
      Serial.printf("min: %.1f - max: &.1f  |  ", _lowerLimit, _upperLimit);
      Serial.printf("current: %.1f  |  ", _currentAngle);
      Serial.printf("upper: %.1f - %.1f  maps to %.1f - %.1f", _lowerLimit, _upperLimit, 0, _sweep_angle * 2.0);
    }
    Serial.println("\n------------------------------------------------------\n\n");
  }

  uint8_t _getZone(float norm, bool print)
  {
    if (print)
      Serial.printf("_getZone()... norm: %.1f \n", norm);
    if (norm >= 0.0 && norm < _sweep_angle)
      return DialZone::BRAKING;
    else if (norm == _sweep_angle)
      return DialZone::IDLE;
    else if (norm > _sweep_angle && norm <= _sweep_angle * 2.0)
      return DialZone::ACCEL;
    else if (norm > _sweep_angle * 2.0 && norm < 210)
      return DialZone::UPPER_LIMIT;
    else
      return DialZone::LOWER_LIMIT;
  }

  float to360(float val)
  {
    if (val < 0.0)
      val = 360.0 + val;
    else if (val > 360.0)
      val = fmod(val, 360.0);
    if (val == 360.0)
      val = 0.0;
    return val;
  }
};
