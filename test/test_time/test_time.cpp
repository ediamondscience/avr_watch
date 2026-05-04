#include <unity.h>
#include <ArduinoFake.h>
#include "timekeeping.h"
#include "timesource.h"

using namespace fakeit;

void setUp(void) {
    ArduinoFake().reset();
}

void tearDown(void) {
    // clean up after each test
}

void test_time_init(void) {
    Timekeeping::init(10, 30);
    Timekeeping::Time t = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t.hours);
    TEST_ASSERT_EQUAL(30, t.minutes);
}

void test_increment_minute(void) {
    Timekeeping::init(10, 30);
    Timekeeping::incrementMinute();
    Timekeeping::Time t = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t.hours);
    TEST_ASSERT_EQUAL(31, t.minutes);
}

void test_increment_minute_rollover(void) {
    Timekeeping::init(10, 59);
    Timekeeping::incrementMinute();
    Timekeeping::Time t = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(11, t.hours);
    TEST_ASSERT_EQUAL(0, t.minutes);
}

void test_increment_hour_rollover(void) {
    Timekeeping::init(12, 59);
    Timekeeping::incrementMinute();
    Timekeeping::Time t = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(1, t.hours);
    TEST_ASSERT_EQUAL(0, t.minutes);
}

void test_minute_ticker_no_increment(void) {
    Timekeeping::init(10, 30);
    When(Method(ArduinoFake(), millis)).Return(0, 59999);
    
    minute_ticker(); // should set lastMinuteMillis to 0
    Timekeeping::Time t1 = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t1.hours);
    TEST_ASSERT_EQUAL(30, t1.minutes);

    minute_ticker(); // should not increment
    Timekeeping::Time t2 = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t2.hours);
    TEST_ASSERT_EQUAL(30, t2.minutes);
}

void test_minute_ticker_increment(void) {
    Timekeeping::init(10, 30);
    When(Method(ArduinoFake(), millis)).Return(0, 60000);

    minute_ticker(); // should set lastMinuteMillis to 0
    Timekeeping::Time t1 = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t1.hours);
    TEST_ASSERT_EQUAL(30, t1.minutes);

    minute_ticker(); // should increment
    Timekeeping::Time t2 = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t2.hours);
    TEST_ASSERT_EQUAL(31, t2.minutes);
}

void test_minute_ticker_increment_multiple(void) {
    Timekeeping::init(10, 30);
    When(Method(ArduinoFake(), millis)).Return(0, 60000, 120000);

    minute_ticker(); // millis = 0
    minute_ticker(); // millis = 60000, increment
    Timekeeping::Time t1 = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t1.hours);
    TEST_ASSERT_EQUAL(31, t1.minutes);

    minute_ticker(); // millis = 120000, increment
    Timekeeping::Time t2 = Timekeeping::getCurrentTime();
    TEST_ASSERT_EQUAL(10, t2.hours);
    TEST_ASSERT_EQUAL(32, t2.minutes);
}


#ifdef ARDUINO
#include <Arduino.h>
void setup() {}
#else
int main() {
#endif
    UNITY_BEGIN();
    RUN_TEST(test_time_init);
    RUN_TEST(test_increment_minute);
    RUN_TEST(test_increment_minute_rollover);
    RUN_TEST(test_increment_hour_rollover);
    RUN_TEST(test_minute_ticker_no_increment);
    RUN_TEST(test_minute_ticker_increment);
    RUN_TEST(test_minute_ticker_increment_multiple);
    UNITY_END();

#ifndef ARDUINO
    return 0;
#endif
}

#ifdef ARDUINO
void loop() {}
#endif

