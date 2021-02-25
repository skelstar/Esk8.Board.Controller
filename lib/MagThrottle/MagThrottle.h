#ifndef Arduino
#include <Arduino.h>
#endif

#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

const char *statePrintFormat = "[State]: %s\n";

State stLowerLimit(
    [] {
      Serial.printf(statePrintFormat, "stLowerLimit");
    },
    NULL, NULL);

State stBraking(
    [] {
      Serial.printf(statePrintFormat, "stBraking");
    },
    NULL, NULL);

State stIdle(
    [] {
      Serial.printf(statePrintFormat, "stIdle");
    },
    NULL, NULL);

State stAccelerating(
    [] {
      Serial.printf(statePrintFormat, "stAccelerating");
    },
    NULL, NULL);

State stUpperLimit(
    [] {
      Serial.printf(statePrintFormat, "stUpperLimit");
    },
    NULL, NULL);

Fsm fsm(&stLowerLimit);

class MagThrottle
{
public:
  void init(AMS_5600 *ams, Fsm *fsm, uint8_t sweep_angle)
  {
    _ams = ams;
    _fsm = fsm;
    _sweep_angle = sweep_angle;

    _throttle_map.init(0, sweep_angle * 2, 0, 255);

    centre();

    _addTransitions();
  }

  void loop()
  {
    _fsm->run_machine();
  }

  uint8_t get(bool enabled = true)
  {
    _current = _convertRawAngleToDegrees(_ams->getRawAngle());

    uint8_t s = _getState();

    switch (s)
    {
    case LOWER_LIMIT:
      _fsm->trigger(TR_LOWER_LIMIT);
      break;
    case BRAKING:
      _fsm->trigger(TR_BRAKING);
      break;
    case IDLE:
      _fsm->trigger(TR_IDLE);
      break;
    case ACCEL:
      _fsm->trigger(TR_ACCEL);
      break;
    case UPPER_LIMIT:
      _fsm->trigger(TR_UPPER_LIMIT);
      break;
    default:
      Serial.printf("OUTOF RANGE: mapping state to trigger\n");
    }

    //     if (_current > _min || (_current > 0 && _current < _centre))
    // {
    //   return DialState::BRAKE;
    //   // angle = _l_map.constrainedMap(_current);
    // }
    // else if (_current == _centre)
    // {
    //   return DialState::IDLE;
    //   // angle = _sweep_angle;
    // }
    // else if (_current > _centre && _current <= _max)
    // {
    //   return DialState::ACCEL;
    //   // angle = _upper_map.constrainedMap(_current);
    // }
    // else if (_current > _max && _current < _max + 180)
    // {
    //   return DialState::UPPER_LIMIT;
    //   // angle = _sweep_angle * 2.0;
    // }
    // else
    // {
    //   return DialState::LOWER_LIMIT;
    //   // angle = 0;
    // }

    float delta = _last - _current;
    float angle = 0;

    uint8_t throttle = _throttle_map.map(angle);

    if (abs(delta) > 0.5)
    {
      Serial.printf("raw: %0.1fdeg   ", _current);
      Serial.printf("mapped to: %0.1fdeg   ", angle);
      Serial.printf("throttle: %d   ", throttle);
      Serial.printf("state: %d   ", _state);
      Serial.printf("state: %s   ", _getDialState(_state));
      Serial.printf("\n");
    }
    _last = _current;

    return throttle;
  }

  void centre(bool print = false)
  {
    _current = _convertRawAngleToDegrees(_ams->getRawAngle());
    _centre = _current;
    _last = _current;
    _max = _centre + _sweep_angle;
    if (_max > 360)
      _max = _max - 360;
    _min = _centre - _sweep_angle;
    if (_min > _centre)
      // wrapped
      _min = 360 + _min;
    _wraps = _centre < _sweep_angle || _centre > 360 - _sweep_angle;

    _state = DialState::IDLE;

    _setMaps(print);
  }

private:
  AMS_5600 *_ams = nullptr;
  Fsm *_fsm;

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

  void _addTransitions()
  {
    // stLowerLimit
    _fsm->add_transition(&stLowerLimit, &stBraking, TR_BRAKING, NULL);
    // stBraking
    _fsm->add_transition(&stBraking, &stIdle, TR_IDLE, NULL);
    _fsm->add_transition(&stBraking, &stLowerLimit, TR_LOWER_LIMIT, NULL);
    _fsm->add_transition(&stBraking, &stAccelerating, TR_ACCEL, NULL);
    // stIdle
    _fsm->add_transition(&stIdle, &stAccelerating, TR_ACCEL, NULL);
    _fsm->add_transition(&stIdle, &stBraking, TR_BRAKING, NULL);
    // stAccelerating
    _fsm->add_transition(&stAccelerating, &stUpperLimit, TR_UPPER_LIMIT, NULL);
    _fsm->add_transition(&stAccelerating, &stIdle, TR_IDLE, NULL);
    _fsm->add_transition(&stAccelerating, &stBraking, TR_BRAKING, NULL);
    // stUpperLimit
    _fsm->add_transition(&stUpperLimit, &stAccelerating, TR_ACCEL, NULL);
  }

  float _convertRawAngleToDegrees(word newAngle)
  {
    /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */
    float retVal = newAngle * 0.087;
    ang = retVal;
    return retVal;
  }

  void _setMaps(bool print = false)
  {
    if (_wraps)
    {
      _l[MIN_IN] = _min;
      _l[MAX_IN] = 360.0;
      _l[MIN_OUT] = 0;
      _l[MAX_OUT] = 360 - _min; // abs(_min);
      _lower_map.init(_l[MIN_IN], _l[MAX_IN], _l[MIN_OUT], _l[MAX_OUT]);

      _u[MIN_IN] = _u[MAX_IN];
      _u[MIN_OUT] = _u[MAX_OUT];
      _u[MIN_OUT] = abs(_min);
      _u[MAX_OUT] = _sweep_angle * 2;
      _upper_map.init(_u[MIN_IN], _u[MAX_IN], _u[MIN_OUT], _u[MAX_OUT]);
    }
    else
    {
      _upper_map.init(_min, _max, 0, _sweep_angle * 2);
    }
    if (print)
      _print();
  }

  void _print()
  {
    if (outlier)
    {
      Serial.printf("WRAPS ");
      Serial.printf("current: %.1f ", _current);
      Serial.printf("lower: %.1f - %.1f => %.1f - %.1f |  ", _l[MIN_IN], _l[MAX_IN], _l[MIN_OUT], _l[MAX_OUT]);
      Serial.printf("upper: %.1f - %.1f => %.1f - %.1f", _u[MIN_IN], _u[MAX_IN], _u[MIN_OUT], _u[MAX_OUT]);
      Serial.println();
    }
    else
    {
      Serial.printf("NORMAL | ");
      Serial.printf("current: %.1f ", _current);
      Serial.printf("upper: %.1f - %.1f ", _min, _max);
      Serial.println();
    }
  }

  uint8_t _getState()
  {
    if (_wraps)
    {
      if (_current > _min || (_current > 0 && _current < _centre))
        return DialState::BRAKING;
      else if (_current == _centre)
        return DialState::IDLE;
      else if (_current > _centre && _current <= _max)
        return DialState::ACCEL;
      else if (_current > _max && _current < _max + 180)
        return DialState::UPPER_LIMIT;
      else
        return DialState::LOWER_LIMIT;
    }
    else
    {
      if (_current > _min && _current < _centre)
        return DialState::BRAKING;
      else if (_current == _centre)
        return DialState::IDLE;
      else if (_current > _centre && _current <= _max)
        return DialState::ACCEL;
      else if (_current > _max && _current < _max + 180)
        return DialState::UPPER_LIMIT;
      else
        return DialState::LOWER_LIMIT;
    }
  }

  FastMap _lower_map, _upper_map, _throttle_map;

  bool _wraps = false;
  uint8_t _sweep_angle = 0;
  float _last = -1,
        _current = 0,
        _centre = 0,
        _max = 0,
        _min = 0;

  enum DialState
  {
    LOWER_LIMIT,
    BRAKING,
    IDLE,
    ACCEL,
    UPPER_LIMIT,
  } _state;

  const char *_getDialState(DialState state)
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
    return "OUT OF RANGE (getDialState())";
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
};