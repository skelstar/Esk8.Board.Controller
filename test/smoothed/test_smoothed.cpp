#include <Arduino.h>
#include <unity.h>
#include <Smoothed.h>

Smoothed <int> sm_throttle;

void setUp()
{
  sm_throttle.clear();
}

void tearDown()
{
}

void test_smoothed_throttle_is_expected()
{ 
  sm_throttle.begin(SMOOTHED_AVERAGE, 10);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);
  sm_throttle.add(127);

  uint8_t actual = sm_throttle.get();
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void setup()
{
  UNITY_BEGIN();
}

void loop()
{
  RUN_TEST(test_smoothed_throttle_is_expected);

  UNITY_END();
}
