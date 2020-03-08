#include <Arduino.h>
#include <unity.h>
#include <Smoother.h>

Smoother sm_throttle(10, 127, 3);

void setUp()
{
  delay(1000);
}

void tearDown()
{
}

void test_smoother_inits_properly()
{
  uint8_t actual = sm_throttle.get();
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_inits_then_adds_255_and_gets_correct_result()
{
  sm_throttle.add(255);

  uint8_t actual = sm_throttle.get();
  uint8_t expected = (uint8_t)((127 * 3 + 255) / 4);

  TEST_ASSERT_EQUAL(expected, actual);
}

// void test_smoothed_throttle_is_expected()
// {
//   sm_throttle.(10);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);
//   sm_throttle.add(127);

//   uint8_t actual = sm_throttle.get();
//   uint8_t expected = 127;

//   TEST_ASSERT_EQUAL(expected, actual);
// }

void setup()
{
  UNITY_BEGIN();

  RUN_TEST(test_smoother_inits_properly);

  UNITY_END();
}

void loop()
{
}
