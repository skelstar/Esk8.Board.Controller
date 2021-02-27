#ifndef Arduino
#include <Arduino.h>
#endif

#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

namespace MagThrottle
{
  const char *statePrintFormat = "[State]: %s\n";

  typedef void (*ThrottleChangedCallback)(uint8_t);

  ThrottleChangedCallback _throttleChangedCb = nullptr;

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
  // _lowerLimit = 0,
  // _upperLimit = 0

  uint8_t _throttle = 127,
          _currentZone = 0;

  void centre(bool print = false);
  void _addTransitions();
  float _convertRawAngleToDegrees(word newAngle);
  uint8_t _getZone(float raw, bool print = false);
  void _setMaps(bool print = false);
  void _print();
  float to360(float val);
  float _normalise(float raw, bool print = false);

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
    TR_UPPER_LIMIT
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
    }
    return "OUT OF RANGE (_getTrigger())";
  }

  bool angleChanged()
  {
    return abs(_currentAngle - _prevAngle) > 0.5;
  }

  FastMap getMap(float raw)
  {
    if (_wraps)
    {
      if (raw >= _l[MIN_IN] && raw <= _l[MAX_IN])
        return _lower_map;
      else if (raw >= _u[MIN_IN] && raw <= _u[MAX_IN])
        return _upper_map;
      else
        Serial.printf("OUT OF RANGE getMap: raw=%0.1f\n", raw);
      return _lower_map;
    }
  }

  State stLowerLimit(
      [] {
        Serial.printf(statePrintFormat, "stLowerLimit");
        _throttle = 0;
        if (_throttleChangedCb != nullptr)
          _throttleChangedCb(_throttle);
      },
      NULL, NULL);

  void updateThrottle()
  {
    if (abs(_currentAngle - _prevAngle) > 0.5)
    {
      FastMap map = getMap(_currentAngle);
      float angle = map.constrainedMap(_currentAngle);
      Serial.printf("angle: %0.1f\n", angle);
      _throttle = _throttle_map.map(angle);

      if (_throttleChangedCb != nullptr)
        _throttleChangedCb(_throttle);
    }
  }

  State stBraking(
      [] {
        Serial.printf(statePrintFormat, "stBraking");
      },
      [] {
        updateThrottle();
      },
      NULL);

  State stIdle(
      [] {
        Serial.printf(statePrintFormat, "stIdle");
        _throttle = 127;
        if (_throttleChangedCb != nullptr)
          _throttleChangedCb(_throttle);
      },
      NULL, NULL);

  State stAccelerating(
      [] {
        Serial.printf(statePrintFormat, "stAccelerating");
      },
      [] {
        updateThrottle();
      },
      NULL);

  State stUpperLimit(
      [] {
        Serial.printf(statePrintFormat, "stUpperLimit");
        _throttle = 255;
        if (_throttleChangedCb != nullptr)
          _throttleChangedCb(_throttle);
      },
      NULL, NULL);

  Fsm fsm(&stLowerLimit);

  void init(float sweep_angle, ThrottleChangedCallback throttleChangedCb)
  {
    _sweep_angle = sweep_angle;
    _throttleChangedCb = throttleChangedCb;

    _throttle_map.init(0, sweep_angle * 2, 0, 255);

    centre(/*print*/ true);

    _addTransitions();
  }

  void loop()
  {
    fsm.run_machine();
  }

  void _triggerFsm(uint8_t zone)
  {
    switch (zone)
    {
    case LOWER_LIMIT:
      fsm.trigger(TR_LOWER_LIMIT);
      break;
    case BRAKING:
      fsm.trigger(TR_BRAKING);
      break;
    case IDLE:
      fsm.trigger(TR_IDLE);
      break;
    case ACCEL:
      fsm.trigger(TR_ACCEL);
      break;
    case UPPER_LIMIT:
      fsm.trigger(TR_UPPER_LIMIT);
      break;
    default:
      Serial.printf("OUTOF RANGE: mapping state to trigger\n");
    }
  }

  void get(bool enabled = true)
  {
    _prevAngle = _currentAngle;
    _currentAngle = _convertRawAngleToDegrees(ams5600.getRawAngle());
    float _mapped = _normalise(_currentAngle);
    _currentZone = _getZone(_currentAngle);
    _triggerFsm(_currentZone);

    // if (abs(delta) > 0.5)
    // {
    //   Serial.printf("raw: %0.1fdeg (%0.1f deg)  ", _currentAngle, _centre);
    //   Serial.printf("mapped to: %0.1fdeg   ", angle);
    //   Serial.printf("throttle: %d   ", throttle);
    //   Serial.printf("state: %d   ", _zone);
    //   Serial.printf("state: %s   ", _getDialZone(_zone));
    //   Serial.printf("\n");
    // }
    // _last = _currentAngle;

    return;
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
  }

  // uint8_t _getState(FsmTrigger trigger)
  // {
  //   switch (_currentZone)
  //   {
  //   case LOWER_LIMIT:
  //     break;
  //   case BRAKING:
  //     if (trigger == TR_IDLE)
  //       _state = IDLE;
  //     else if (trigger == TR_LOWER_LIMIT)
  //       _state = LOWER_LIMIT;
  //     else if (trigger == TR_ACCEL)
  //       _state = ACCEL;
  //     break;
  //   case IDLE:
  //     if (trigger == TR_ACCEL)
  //       _state = ACCEL;
  //     else if (trigger == TR_BRAKING)
  //       _state = BRAKING;
  //     break;
  //   case ACCEL:
  //     if (trigger == TR_UPPER_LIMIT)
  //       _state = UPPER_LIMIT;
  //     else if (trigger == TR_IDLE)
  //       _state = IDLE;
  //     else if (trigger == TR_BRAKING)
  //       _state = BRAKING;
  //     break;
  //   case UPPER_LIMIT:
  //     if (trigger == TR_ACCEL)
  //       _state = ACCEL;
  //     break;
  //   }
  // }

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
      // _zone = DialZone::IDLE;

      // Serial.printf("_setBoundaries: min: %.1f   _centre:%.1f  _offset: %.1f  max: %.1f \n", _lowerLimit, _centre, _offset, _upperLimit);
    }

    // raw is 0.0 -> 360.0
  }

  float _normalise(float raw, bool print)
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

    // _setMaps(print);
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

  // float delta(float a, float b)
  // {
  //   float d1 = abs(a - b);
  //   if (a > b)
  //   {
  //     float d2 = abs(a - (b + 360.0));
  //     return d1 > d2 ? d2 : d1;
  //   }
  //   else
  //   {
  //     float d2 = abs(b - (a + 360.0));
  //     return d1 > d2 ? d2 : d1;
  //   }
  // }

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

  float toRel(float val, float centre)
  {
    float delta = abs(centre - val);
    if (delta < 180.0)
    {
      Serial.printf(" < 180\n");
      return 0.0;
    }
    Serial.printf(" >= 180\n");
    return 0.0;
  }
};
