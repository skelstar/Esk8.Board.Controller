#include <Arduino.h>
#include <unity.h>
#include <Smoother.h>

Smoother smoother(10, 127, 3);

void setUp()
{
  // delay(1000);
}

void tearDown()
{
}

void test_smoother_inits_properly()
{
  uint8_t actual = smoother.get();
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_inits_then_adds_255_and_gets_correct_result()
{
  smoother.add(255);

  uint8_t actual = smoother.get();
  uint8_t expected = (uint8_t)((127 * 3 + 255) / 4);

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_we_add_10_items_then_one_returns_correct_result()
{
  for (int i = 0; i < 10; i++)
  {
    smoother.add(127);
  }
  smoother.add(200);
  smoother.add(127);
  smoother.add(127);
  smoother.add(127);

  uint8_t actual = smoother.get();
  uint8_t expected = (uint8_t)(((127 * 9) + 200) / 10);

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_we_add_10_items_using_clear_then_returns_correct_result()
{
  smoother.clear(127, 10);

  uint8_t actual = smoother.get();
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_we_get_last_then_returns_correct_result()
{
  smoother.clear(0, 5);

  smoother.add(1);
  smoother.add(2);
  smoother.add(3);

  uint8_t actual = smoother.getLast();
  uint8_t expected = 3;

  smoother.printBuffer();

  TEST_ASSERT_EQUAL(expected, actual);
}

void setup()
{
  UNITY_BEGIN();

  RUN_TEST(test_smoother_inits_properly);
  RUN_TEST(test_when_we_add_10_items_then_one_returns_correct_result);
  RUN_TEST(test_when_we_add_10_items_using_clear_then_returns_correct_result);
  RUN_TEST(test_when_we_get_last_then_returns_correct_result);

  UNITY_END();
}

void loop()
{
}
