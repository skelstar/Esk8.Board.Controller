#include <Arduino.h>
#include <unity.h>
#include <RetryLoggerLib.h>

RetryLoggerLib retry_logger(/*length*/ 3);

void setUp()
{
  retry_logger.reset();
}

void tearDown()
{
}

void test_sum_retries_is_expected()
{ 
  retry_logger.log(1);
  retry_logger.log(1);
  retry_logger.log(1);
  retry_logger.log(1);
  retry_logger.log(1);
  retry_logger.log(1);
  retry_logger.log(0);
  retry_logger.log(0);

  uint8_t actual = retry_logger.get_sum_retries();
  uint8_t expected = 1;

  TEST_ASSERT_EQUAL(expected, actual);
}

void test_retry_rate_is_expected()
{
  retry_logger.log(1);
  retry_logger.log(1);
  retry_logger.log(0);
  retry_logger.log(0);

  TEST_ASSERT_EQUAL(1/3.0, retry_logger.get_retry_rate());
}

void setup()
{
  UNITY_BEGIN();
}

void loop()
{
  RUN_TEST(test_sum_retries_is_expected);

  RUN_TEST(test_retry_rate_is_expected);

  UNITY_END();
}
