
#ifndef Preferences_h
#include <Preferences.h>
// https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
#endif

Preferences preferences;

void storeUInt8(const char* name_space, const char* name, uint8_t value) {
	preferences.begin(name_space, false);	// r/w
	preferences.putUChar(name, value);
	preferences.end();
}

int recallUInt8(const char* name_space, char* name)  {
	preferences.begin(name_space, false);	// r/w
	uint8_t result = preferences.getUChar(name, 0);
	preferences.end();
	return result;
}

void storeFloat(const char* name_space, char* name, float value) {
	preferences.begin(name_space, false);	// r/w
	preferences.putFloat(name, value);
	preferences.end();
}

float recallFloat(const char* name_space, char* name) {
	preferences.begin(name_space, false);	// r/w
	float result = preferences.getFloat(name, 0.0);
	preferences.end();
	return result;
}

//------------------------------------------------------------------------------------------

class Trip
{
  public:

    struct TripType
    {
      float ampHours;
      float odometer;
    } data;

    Trip()
    {
      data.ampHours = recallFloat("LAST TRIP", AMP_HOURS);
      data.odometer = recallFloat("LAST TRIP", ODOMETER);
      _updateToDate = true;
    }

    void save(VescData data)
    {
			DEBUG("saved last_trip");
      storeFloat("LAST TRIP", ODOMETER, data.odometer);
      storeFloat("LAST TRIP", AMP_HOURS, data.ampHours);
      _updateToDate = false;
    }

    TripType recall()
    {
      struct TripType result;

      if (_updateToDate == false)
      {
        _updateToDate = true;
        data.ampHours = recallFloat("LAST TRIP", AMP_HOURS);
        data.odometer = recallFloat("LAST TRIP", ODOMETER);
				DEBUG("updated last_trip");
      }
      
      result.odometer =  recallFloat("LAST TRIP", ODOMETER);
      return result;
    }

  private:
    bool _updateToDate = false;

    char *AMP_HOURS = "AMP HOURS";
    char *ODOMETER = "ODOMETER";

};