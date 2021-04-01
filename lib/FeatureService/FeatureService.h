#pragma once

enum FeatureType
{
  CRUISE_CONTROL,
  PUSH_TO_START
};

class FeatureServiceClass
{

public:
  FeatureServiceClass()
  {
    set(CRUISE_CONTROL, FEATURE_CRUISE_CONTROL);
    set(PUSH_TO_START, FEATURE_PUSH_TO_START);
  }

  template <class T>
  void set(FeatureType feature, T value)
  {
    switch (feature)
    {
    case CRUISE_CONTROL:
      _featureCruiseControl = value;
      break;
    case PUSH_TO_START:
      _featurePushToStart = value;
      break;
    }
  }

  template <class T>
  T get(FeatureType feature)
  {
    switch (feature)
    {
    case CRUISE_CONTROL:
      return _featureCruiseControl;
    case PUSH_TO_START:
      return _featurePushToStart;
    }
    return NULL;
  }

private:
  bool _featureCruiseControl;
  bool _featurePushToStart;
} featureService;
