#include <Arduino.h>
#include <unity.h>

#include <TriggerLib.h>

TriggerLib trigger(0);

void setUp()
{
  trigger.initialise();
  trigger.set_deadman_pin(10);
}

void tearDown()
{
}

void test_state_transitions()
{
  // IDLE->IDLE
  trigger.t_state = trigger.IDLE_STATE;
  trigger.deadman_held = false;
  trigger.max_throttle = 127;
  trigger.manage_state(/*raw*/127);
  TEST_ASSERT_EQUAL(trigger.IDLE_STATE, trigger.t_state);

  // IDLE->GO
  trigger.t_state = trigger.IDLE_STATE;
  trigger.deadman_held = true;
  trigger.max_throttle = 127;
  trigger.manage_state(/*raw*/127);
  TEST_ASSERT_EQUAL(trigger.GO_STATE, trigger.t_state);

  // GO->IDLE
  trigger.t_state = trigger.GO_STATE;
  trigger.deadman_held = false;
  trigger.max_throttle = 127;
  trigger.manage_state(/*raw*/127);
  TEST_ASSERT_EQUAL(trigger.IDLE_STATE, trigger.t_state);

  // GO->WAIT
  trigger.t_state = trigger.GO_STATE;
  trigger.deadman_held = false;
  trigger.max_throttle = 160;
  trigger.manage_state(/*raw*/170);
  TEST_ASSERT_EQUAL(trigger.WAIT_STATE, trigger.t_state);

  // WAIT->IDLE
  trigger.t_state = trigger.WAIT_STATE;
  trigger.deadman_held = false;
  trigger.max_throttle = 127;
  trigger.manage_state(/*raw*/127);
  TEST_ASSERT_EQUAL(trigger.IDLE_STATE, trigger.t_state);
}

void test_when_waiting_and_braking_its_not_waiting_anymore()
{
  trigger.t_state = trigger.WAIT_STATE;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 119);
  uint8_t expected = false;

  TEST_ASSERT_EQUAL(expected, trigger.waiting_for_idle_throttle);
}

void test_when_waiting_and_not_braking_it_is_still_waiting()
{
  trigger.t_state = trigger.WAIT_STATE;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 130);
  uint8_t expected = true;

  TEST_ASSERT_EQUAL(trigger.WAIT_STATE, trigger.t_state);
}

void test_is_waiting_is_reset_when_idle_and_is_GO_STATE()
{
  trigger.t_state = trigger.WAIT_STATE;
  trigger.deadman_held = true;
  
  uint8_t throttle = trigger.make_throttle_safe(/*raw*/ 127);
  uint8_t expected = trigger.GO_STATE;

  TEST_ASSERT_EQUAL(expected, trigger.t_state);
}

void test_when_deadman_not_held_it_still_brakes()
{
  trigger.deadman_held = false;
  trigger.waiting_for_idle_throttle = false;
  // trigger.max_throttle = 127;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 120);
  uint8_t expected = 120;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_deadman_is_held_and_not_waiting_it_accelerates()
{
  trigger.t_state = trigger.GO_STATE;
  trigger.deadman_held = true;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 180);
  uint8_t expected = 180;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_deadman_is_held_and_is_waiting_it_limits_to_max()
{
  trigger.t_state = trigger.WAIT_STATE;
  trigger.deadman_held = true;
  trigger.max_throttle = 180;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 200);
  uint8_t expected = 180;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_is_waiting_it_reduces_max_to_raw_if_raw_is_less()
{
  trigger.t_state = trigger.WAIT_STATE;
  trigger.max_throttle = 180;
  trigger.deadman_held = true;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 160);
  uint8_t expected = 160;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_deadman_off_and_accel_it_set_to_idle_throttle()
{
  trigger.t_state = trigger.GO_STATE;
  trigger.deadman_held = false;

  uint8_t actual = trigger.make_throttle_safe(/*raw*/ 160);
  uint8_t expected = 160;

  TEST_ASSERT_EQUAL(expected, actual);
}

void setup()
{
  Serial.begin(115200);
  UNITY_BEGIN();

  RUN_TEST(test_state_transitions);
  RUN_TEST(test_when_waiting_and_braking_its_not_waiting_anymore);
  RUN_TEST(test_when_waiting_and_not_braking_it_is_still_waiting);
  RUN_TEST(test_is_waiting_is_reset_when_idle_and_is_GO_STATE);
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
