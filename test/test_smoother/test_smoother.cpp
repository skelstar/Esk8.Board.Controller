#include <Arduino.h>
#include <unity.h>
#include <Smoother.h>

void setUp()
{
  // delay(1000);
}

void tearDown()
{
}

void test_smoother_inits_properly()
{
  const byte factor = 5;
  Smoother *sm = new Smoother(factor, 127);

  uint8_t actual = sm->get();
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_inits_then_adds_255_and_gets_correct_result()
{
  const byte factor = 5;
  Smoother *sm = new Smoother(factor, 127);

  sm->add(255);

  uint8_t actual = sm->get();
  uint8_t expected = (uint8_t)((127 * 3 + 255) / 4);

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_we_add_n_items_then_one_returns_correct_result()
{
  const byte factor = 5;
  Smoother *sm = new Smoother(factor, 127);
  sm->add(127);
  sm->add(127);
  sm->add(127);
  sm->add(127);
  sm->add(127);
  sm->add(200);

  sm->printBuffer();

  uint8_t actual = sm->get();
  uint8_t expected = (uint8_t)(((127 * (factor - 1)) + 200) / factor);

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_we_add_n_items_using_clear_then_returns_correct_result()
{
  const byte factor = 5;
  Smoother *sm = new Smoother(factor, 127);
  sm->add(255);

  sm->clear(127);

  uint8_t actual = sm->get();
  uint8_t expected = 127;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_we_get_last_then_returns_correct_result()
{
  const byte factor = 5;
  Smoother *sm = new Smoother(factor, 127);

  sm->clear(0);

  sm->add(1);
  sm->add(2);
  sm->add(3);

  uint8_t actual = sm->getLast();
  uint8_t expected = 3;

  sm->printBuffer();

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_when_values_wrap_around_then_returns_correct_result()
{
  const byte factor = 5;
  Smoother *sm = new Smoother(factor, 127);

  sm->clear(0);

  for (int i = 0; i < 5 + 2; i++)
  {
    sm->add(i);
  }

  uint8_t actual = sm->getLast();
  uint8_t expected = 6;

  sm->printBuffer();

  TEST_ASSERT_EQUAL(expected, actual);
}

void setup()
{
  UNITY_BEGIN();

  RUN_TEST(test_smoother_inits_properly);
  RUN_TEST(test_when_we_add_n_items_then_one_returns_correct_result);
  RUN_TEST(test_when_we_add_n_items_using_clear_then_returns_correct_result);
  RUN_TEST(test_when_we_get_last_then_returns_correct_result);
  RUN_TEST(test_when_values_wrap_around_then_returns_correct_result);

  UNITY_END();
}

void loop()
{
}
