#include <Arduino.h>
#include <unity.h>

#include <Wire.h>
#include <AS5600.h>

AMS_5600 ams5600;

#include <MagThrottle.h>

// runs every test
void setUp()
{
}

// runs every test
void tearDown()
{
}

void test_setBoundaries_at_centre_0()
{
  using namespace MagThrottle;

  _centre = 0.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(330.0, _lowerLimit);
  TEST_ASSERT_EQUAL(0.0, _centre);
  TEST_ASSERT_EQUAL(30.0, _upperLimit);
  TEST_ASSERT_EQUAL(true, _wraps);
}

void test_setBoundaries_at_centre_10()
{
  using namespace MagThrottle;

  _centre = 10.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(340.0, _lowerLimit);
  TEST_ASSERT_EQUAL(10.0, _centre);
  TEST_ASSERT_EQUAL(40.0, _upperLimit);
  TEST_ASSERT_EQUAL(true, _wraps);
}

void test_setBoundaries_at_centre_30()
{
  using namespace MagThrottle;

  _centre = 30.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(0.0, _lowerLimit);
  TEST_ASSERT_EQUAL(30.0, _centre);
  TEST_ASSERT_EQUAL(60.0, _upperLimit);
  TEST_ASSERT_EQUAL(false, _wraps);
}

void test_setBoundaries_at_centre_340()
{
  using namespace MagThrottle;

  _centre = 340.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(310.0, _lowerLimit);
  TEST_ASSERT_EQUAL(340.0, _centre);
  TEST_ASSERT_EQUAL(10.0, _upperLimit);
  TEST_ASSERT_EQUAL(true, _wraps);
}

void test_setBoundaries_at_centre_358()
{
  using namespace MagThrottle;

  _centre = 358.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(328.0, _lowerLimit);
  TEST_ASSERT_EQUAL(358.0, _centre);
  TEST_ASSERT_EQUAL(28.0, _upperLimit);
  TEST_ASSERT_EQUAL(true, _wraps);
}

// void test_toRel()
// {
//   using namespace MagThrottle;

//   TEST_ASSERT_EQUAL(-60.0, toRel(330, 30.0));
//   TEST_ASSERT_EQUAL(40.0, toRel(70.0, 30.0));
//   TEST_ASSERT_EQUAL(-30.0, toRel(330, 0.0));
//   TEST_ASSERT_EQUAL(70.0, toRel(70.0, 0.0));
// }

// void test_setMaps_with_wrapping_low_side()
// {
//   using namespace MagThrottle;

//   _centre = 20.0;
//   _setBoundaries(/*current*/ _centre);
//   _setMaps();

//   TEST_ASSERT_EQUAL(0.0, _upper_map.map(350.0));
//   TEST_ASSERT_EQUAL(30.0, _upper_map.map(20.0));
//   TEST_ASSERT_EQUAL(0.0, _upper_map.map(350.0));
//   TEST_ASSERT_EQUAL(0.0, _upper_map.map(350.0));
// }

// void test_setMaps_with_wrapping_high_side()
// {
//   using namespace MagThrottle;

//   _centre = -10.0;
//   _setBoundaries(/*current*/ _centre);
//   _setMaps();

//   TEST_ASSERT_EQUAL(_l[MIN_OUT], 0.0);
//   TEST_ASSERT_EQUAL(_l[MAX_OUT], 40.0);
//   TEST_ASSERT_EQUAL(_u[MIN_OUT], 40.0);
//   TEST_ASSERT_EQUAL(_u[MAX_OUT], 60.0);
// }

// void test_delta()
// {
//   using namespace MagThrottle;

//   TEST_ASSERT_EQUAL(30.0, delta(0.0, 30.0));
//   TEST_ASSERT_EQUAL(30.0, delta(340.0, 10.0));
//   TEST_ASSERT_EQUAL(30.0, delta(320.0, 350.0));
//   TEST_ASSERT_EQUAL(30.0, delta(20.0, 350.0));
//   TEST_ASSERT_EQUAL(30.0, delta(40.0, 10.0));
//   TEST_ASSERT_EQUAL(30.0, delta(60.0, 30.0));
// }

void test_normaliseAngle_centre_at_0()
{
  using namespace MagThrottle;

  _centre = 0.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(62.0, _normaliseTo0toMax(32.0));   // 2 degs past upperLimit
  TEST_ASSERT_EQUAL(359.0, _normaliseTo0toMax(329.0)); // 1 deg before lower limit
  TEST_ASSERT_EQUAL(19.0, _normaliseTo0toMax(349.0));  //
  TEST_ASSERT_EQUAL(0.0, _normaliseTo0toMax(330.0));   // lower limit
  TEST_ASSERT_EQUAL(10.0, _normaliseTo0toMax(340.0));
  TEST_ASSERT_EQUAL(40.0, _normaliseTo0toMax(10.0));
  TEST_ASSERT_EQUAL(30.0, _normaliseTo0toMax(0.0));
}

void test_normaliseAngle_centre_at_20()
{
  using namespace MagThrottle;

  _centre = 20.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(62.0, _normaliseTo0toMax(52.0));
  TEST_ASSERT_EQUAL(359.0, _normaliseTo0toMax(349.0));
  TEST_ASSERT_EQUAL(0.0, _normaliseTo0toMax(350.0));
  TEST_ASSERT_EQUAL(20.0, _normaliseTo0toMax(10.0));
  TEST_ASSERT_EQUAL(40.0, _normaliseTo0toMax(30.0));
  TEST_ASSERT_EQUAL(30.0, _normaliseTo0toMax(20.0));
}

void test_normaliseAngle_centre_at_90()
{
  using namespace MagThrottle;

  _centre = 90.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(330.0, _normaliseTo0toMax(30.0));
  TEST_ASSERT_EQUAL(0.0, _normaliseTo0toMax(60.0));
  TEST_ASSERT_EQUAL(30.0, _normaliseTo0toMax(90.0));
  TEST_ASSERT_EQUAL(60.0, _normaliseTo0toMax(120.0));
  TEST_ASSERT_EQUAL(90.0, _normaliseTo0toMax(150.0));
  TEST_ASSERT_EQUAL(120.0, _normaliseTo0toMax(180.0));
}

void test_normaliseAngle_centre_at_150()
{
  using namespace MagThrottle;

  _centre = 150.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(_sweep_angle, _normaliseTo0toMax(150.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 10.0), _normaliseTo0toMax(160.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 40.0), _normaliseTo0toMax(190.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 120.0), _normaliseTo0toMax(270.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 200.0), _normaliseTo0toMax(350.0));
}

void test_normaliseAngle_centre_at_350()
{
  using namespace MagThrottle;

  _centre = 350.0;
  _setBoundaries(/*current*/ _centre);

  TEST_ASSERT_EQUAL(350.0, _normaliseTo0toMax(310.0));
  TEST_ASSERT_EQUAL(0.0, _normaliseTo0toMax(320.0));
  TEST_ASSERT_EQUAL(_sweep_angle - 20.0, _normaliseTo0toMax(330.0));
  TEST_ASSERT_EQUAL(_sweep_angle - 10.0, _normaliseTo0toMax(340.0));
  TEST_ASSERT_EQUAL(_sweep_angle, _normaliseTo0toMax(350.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 10.0), _normaliseTo0toMax(0.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 30.0), _normaliseTo0toMax(20.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 120.0), _normaliseTo0toMax(110.0));
  TEST_ASSERT_EQUAL(to360(_sweep_angle + 200.0), _normaliseTo0toMax(190.0));
}

void test_getZone_at_0()
{
  using namespace MagThrottle;

  _centre = 0.0;
  _setBoundaries(/*current*/ _centre);

  bool print = false;

  TEST_ASSERT_EQUAL(DialZone::LOWER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(329.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(359.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(330.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::IDLE, _getZone(/*norm*/ _normaliseTo0toMax(0.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(1.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(30.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::UPPER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(31.0, print), print));
}

void test_getZone_at_20()
{
  using namespace MagThrottle;

  _centre = 20.0;
  _setBoundaries(/*current*/ _centre);

  bool print = false;

  TEST_ASSERT_EQUAL(DialZone::LOWER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(349.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(19.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(351.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::IDLE, _getZone(/*norm*/ _normaliseTo0toMax(20.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(21.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(50.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::UPPER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(51.0, print), print));
}

void test_getZone_at_90()
{
  using namespace MagThrottle;

  _centre = 90.0;
  _setBoundaries(/*current*/ _centre);

  bool print = false;

  TEST_ASSERT_EQUAL(DialZone::LOWER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(59.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(60.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(89.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::IDLE, _getZone(/*norm*/ _normaliseTo0toMax(90.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(91.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(120.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::UPPER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(121.0, print), print));
}

void test_getZone_at_350()
{
  using namespace MagThrottle;

  _centre = 350.0;
  _setBoundaries(/*current*/ _centre);

  bool print = false;

  TEST_ASSERT_EQUAL(DialZone::LOWER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(319.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(320.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::BRAKING, _getZone(/*norm*/ _normaliseTo0toMax(349.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::IDLE, _getZone(/*norm*/ _normaliseTo0toMax(350.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(351.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::ACCEL, _getZone(/*norm*/ _normaliseTo0toMax(20.0, print), print));
  TEST_ASSERT_EQUAL(DialZone::UPPER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(21.0, print), print));
}

void test_1()
{
  using namespace MagThrottle;

  // MagThrottle::init(30.0, [](uint8_t t) {
  // });

  // _centre = 90.0;
  // _setBoundaries(/*current*/ _centre);

  // MagThrottle::loop();

  // float norm = _normaliseTo0toMax(100.0);

  bool print = false;

  TEST_ASSERT_EQUAL(DialZone::LOWER_LIMIT, _getZone(/*norm*/ _normaliseTo0toMax(319.0, print), print));
}

void setup()
{
  delay(2000);
  UNITY_BEGIN();

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  MagThrottle::_sweep_angle = 30.0;

  // RUN_TEST(test_setBoundaries_at_centre_0);
  // RUN_TEST(test_setBoundaries_at_centre_10);
  // RUN_TEST(test_setBoundaries_at_centre_30);
  // RUN_TEST(test_setBoundaries_at_centre_340);
  // RUN_TEST(test_setBoundaries_at_centre_358);

  // RUN_TEST(test_toRel);

  // RUN_TEST(test_normaliseAngle_centre_at_0);
  // RUN_TEST(test_normaliseAngle_centre_at_20);
  // RUN_TEST(test_normaliseAngle_centre_at_90);
  // RUN_TEST(test_normaliseAngle_centre_at_150);
  // RUN_TEST(test_normaliseAngle_centre_at_350);

  // RUN_TEST(test_getZone_at_0);
  // RUN_TEST(test_getZone_at_20);
  // RUN_TEST(test_getZone_at_90);
  RUN_TEST(test_getZone_at_350);

  UNITY_END();
}

void loop()
{
}
