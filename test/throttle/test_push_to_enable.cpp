#include <Arduino.h>
#include <unity.h>

#include <TriggerLib.h>

TriggerLib trigger(0);

void setUp()
{
}

void tearDown()
{
}

void test_when_moving_and_accel_then_accel()
{
  uint8_t throttle = trigger.get_FEATURE_PUSH_TO_START_throttle(130, /*moving*/ true);
  uint8_t expected = 130;

  TEST_ASSERT_EQUAL(expected, throttle);
}

void test_when_moving_and_braking_then_will_brake()
{
  uint8_t throttle = trigger.get_FEATURE_PUSH_TO_START_throttle(100, /*moving*/ true);
  uint8_t expected = 100;

  TEST_ASSERT_EQUAL(expected, throttle);
}

void test_when_not_moving_then_wont_accel()
{
  uint8_t throttle = trigger.get_FEATURE_PUSH_TO_START_throttle(130, /*moving*/ false);
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, throttle);
}

void test_when_not_moving_and_braking_then_will_brake()
{
  uint8_t throttle = trigger.get_FEATURE_PUSH_TO_START_throttle(100, /*moving*/ false);
  uint8_t expected = 100;

  TEST_ASSERT_EQUAL(expected, throttle);
}

void setup()
{
  UNITY_BEGIN();

  RUN_TEST(test_when_moving_and_accel_then_accel);
  RUN_TEST(test_when_moving_and_braking_then_will_brake);
  RUN_TEST(test_when_not_moving_then_wont_accel);
  RUN_TEST(test_when_not_moving_and_braking_then_will_brake);

  UNITY_END();
}

void loop()
{
}
