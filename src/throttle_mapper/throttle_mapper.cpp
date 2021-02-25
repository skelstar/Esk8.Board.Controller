#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>
#include <throttle.h>

ThrottleClass throttle;

void setup()
{

  delay(3000); // for comms reasons
  Serial.begin(115200);
  Serial.printf("Ready\n");

  throttle.init(/*pin*/ THROTTLE_PIN, [](uint8_t throttle) {
    // Serial.printf("throttle changed: %d (cruise: %d)\n",
    //               throttle,
    //               controller_packet.cruise_control);
  });
}

elapsedMillis since_read_angle;

void loop()
{
  if (since_read_angle > 200)
  {
    since_read_angle = 0;
    uint8_t mapped = throttle.get();
    Serial.printf("raw: %d mapped: %d\n", throttle.getRaw(), mapped);
  }
}