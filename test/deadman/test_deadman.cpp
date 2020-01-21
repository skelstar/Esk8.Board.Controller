#include <Arduino.h>
#include <unity.h>

#include <TriggerLib.h>
bool waiting_for_idle_throttle = false;
uint8_t max_throttle = 127;

// //-----------------------------------------------------
// uint8_t make_throttle_safe(uint8_t raw, bool deadman_held)
// {
//   bool braking_or_idle = raw <= 127;
//   uint8_t result = 127;

//   if (waiting_for_idle_throttle && braking_or_idle)
//   {
//     waiting_for_idle_throttle = false;
//   }

//   if (deadman_held)
//   {
//     result = raw;
//     if (waiting_for_idle_throttle)
//     {
//       result = raw <= max_throttle
//                    ? raw
//                    : max_throttle;
//       max_throttle = raw;
//     }
//   }
//   else
//   {
//     result = braking_or_idle ? raw
//                              : 127;
//   }
//   return result;
// }
//-----------------------------------------------------

TriggerLib trigger(0);

void setUp()
{
  trigger.initialise();
  trigger.set_deadman_pin(10);
}

void tearDown()
{
}

void test_waiting_for_idle_is_reset_when_idle()
{
  trigger.waiting_for_idle_throttle = true;
  // trigger.max_throttle = 127;
  
  uint8_t throttle = trigger.make_throttle_safe(/*raw*/ 127);
  uint8_t expected = false;

  TEST_ASSERT_EQUAL(expected, waiting_for_idle_throttle);
}

void test_when_deadman_not_held_it_still_brakes()
{
  trigger.waiting_for_idle_throttle = false;
  // trigger.max_throttle = 127;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 120);
  uint8_t expected = 120;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_deadman_is_held_and_not_waiting_it_accelerates()
{
  trigger.waiting_for_idle_throttle = false;
  trigger.deadman_held = true;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 180);
  uint8_t expected = 180;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_deadman_is_held_and_is_waiting_it_limits_to_max()
{
  trigger.waiting_for_idle_throttle = true;
  trigger.deadman_held = true;
  trigger.max_throttle = 180;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 200);
  uint8_t expected = 180;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_is_waiting_it_reduces_max_to_raw_if_raw_is_less()
{
  trigger.waiting_for_idle_throttle = true;
  trigger.max_throttle = 180;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 160);
  uint8_t expected = 160;

  TEST_ASSERT_EQUAL(expected, max_throttle);
}

void test_when_deadman_off_and_accel_it_set_to_idle_throttle()
{
  trigger.waiting_for_idle_throttle = false;
  trigger.max_throttle = 180;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 160);
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void setup()
{
  Serial.begin(115200);
  UNITY_BEGIN();

  RUN_TEST(test_waiting_for_idle_is_reset_when_idle);
  RUN_TEST(test_when_deadman_not_held_it_still_brakes);
  RUN_TEST(test_when_deadman_is_held_and_not_waiting_it_accelerates);
  RUN_TEST(test_when_deadman_is_held_and_is_waiting_it_limits_to_max);
  RUN_TEST(test_when_is_waiting_it_reduces_max_to_raw_if_raw_is_less);
  RUN_TEST(test_when_deadman_off_and_accel_it_set_to_idle_throttle);

  UNITY_END();
}

void loop()
{
}
