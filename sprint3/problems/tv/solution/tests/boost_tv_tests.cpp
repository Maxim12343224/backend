#define BOOST_TEST_MODULE TV tests
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sstream>

#include "../src/tv.h"
#include "boost_test_helpers.h"

struct TVFixture {
    TV tv;
};
BOOST_FIXTURE_TEST_SUITE(TV_, TVFixture)
BOOST_AUTO_TEST_CASE(is_off_by_default) {
    BOOST_TEST(!tv.IsTurnedOn());
}
BOOST_AUTO_TEST_CASE(doesnt_show_any_channel_by_default) {
    BOOST_TEST(!tv.GetChannel().has_value());
}
BOOST_AUTO_TEST_CASE(cant_select_any_channel_when_it_is_off) {
    BOOST_CHECK_THROW(tv.SelectChannel(10), std::logic_error);
    BOOST_TEST(tv.GetChannel() == std::nullopt);
    tv.TurnOn();
    BOOST_TEST(tv.GetChannel() == 1);
}

// Тестовый стенд "Включенный телевизор" унаследован от TVFixture.
struct TurnedOnTVFixture : TVFixture {
    TurnedOnTVFixture() {
        tv.TurnOn();
    }
};

BOOST_FIXTURE_TEST_SUITE(After_turning_on_, TurnedOnTVFixture)
BOOST_AUTO_TEST_CASE(shows_channel_1) {
    BOOST_TEST(tv.IsTurnedOn());
    BOOST_TEST(tv.GetChannel() == 1);
}
BOOST_AUTO_TEST_CASE(can_be_turned_off) {
    tv.TurnOff();
    BOOST_TEST(!tv.IsTurnedOn());
    BOOST_TEST(tv.GetChannel() == std::nullopt);
}
BOOST_AUTO_TEST_CASE(can_select_channel_from_1_to_99) {
    tv.SelectChannel(1);
    BOOST_TEST(tv.GetChannel() == 1);
    
    tv.SelectChannel(99);
    BOOST_TEST(tv.GetChannel() == 99);
    
    BOOST_CHECK_THROW(tv.SelectChannel(0), std::out_of_range);
    BOOST_CHECK_THROW(tv.SelectChannel(100), std::out_of_range);
}
BOOST_AUTO_TEST_CASE(remembers_channel_after_turning_off) {
    tv.SelectChannel(42);
    tv.TurnOff();
    BOOST_TEST(!tv.GetChannel().has_value());
    
    tv.TurnOn();
    BOOST_TEST(tv.GetChannel() == 42);
}
BOOST_AUTO_TEST_CASE(select_previous_channel_switches_between_last_two_channels) {
    tv.SelectChannel(5);
    tv.SelectChannel(10);
    BOOST_TEST(tv.GetChannel() == 10);
    
    tv.SelectLastViewedChannel();
    BOOST_TEST(tv.GetChannel() == 5);
    
    tv.SelectLastViewedChannel();
    BOOST_TEST(tv.GetChannel() == 10);
}
BOOST_AUTO_TEST_CASE(cannot_select_previous_channel_when_off) {
    tv.TurnOff();
    BOOST_CHECK_THROW(tv.SelectLastViewedChannel(), std::logic_error);
}
BOOST_AUTO_TEST_CASE(select_same_channel_does_nothing) {
    tv.SelectChannel(15);
    int initial_channel = tv.GetChannel().value();
    
    tv.SelectChannel(15);
    BOOST_TEST(tv.GetChannel() == initial_channel);
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()