#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "../src/tv.h"

class TVByDefault : public testing::Test {
protected:
    TV tv_;
};
TEST_F(TVByDefault, IsOff) {
    EXPECT_FALSE(tv_.IsTurnedOn());
}
TEST_F(TVByDefault, DoesntShowAChannelWhenItIsOff) {
    EXPECT_FALSE(tv_.GetChannel().has_value());
}
TEST_F(TVByDefault, CantSelectAnyChannel) {
    EXPECT_THROW(tv_.SelectChannel(10), std::logic_error);
    EXPECT_EQ(tv_.GetChannel(), std::nullopt);
    tv_.TurnOn();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

class TurnedOnTV : public TVByDefault {
protected:
    void SetUp() override {
        tv_.TurnOn();
    }
};
TEST_F(TurnedOnTV, ShowsChannel1) {
    EXPECT_TRUE(tv_.IsTurnedOn());
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}
TEST_F(TurnedOnTV, AfterTurningOffTurnsOffAndDoesntShowAnyChannel) {
    tv_.TurnOff();
    EXPECT_FALSE(tv_.IsTurnedOn());
    EXPECT_EQ(tv_.GetChannel(), std::nullopt);
}
TEST_F(TurnedOnTV, CanSelectChannelFrom1To99) {
    tv_.SelectChannel(1);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
    
    tv_.SelectChannel(99);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(99));
    
    EXPECT_THROW(tv_.SelectChannel(0), std::out_of_range);
    EXPECT_THROW(tv_.SelectChannel(100), std::out_of_range);
}
TEST_F(TurnedOnTV, RemembersChannelAfterTurningOffAndOn) {
    tv_.SelectChannel(42);
    tv_.TurnOff();
    EXPECT_FALSE(tv_.GetChannel().has_value());
    
    tv_.TurnOn();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(42));
}
TEST_F(TurnedOnTV, SelectPreviousChannelSwitchesBetweenLastTwoChannels) {
    tv_.SelectChannel(5);
    tv_.SelectChannel(10);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(10));
    
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(5));
    
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(10));
}
TEST_F(TurnedOnTV, CannotSelectPreviousChannelWhenOff) {
    tv_.TurnOff();
    EXPECT_THROW(tv_.SelectLastViewedChannel(), std::logic_error);
}
TEST_F(TurnedOnTV, SelectSameChannelDoesNothing) {
    tv_.SelectChannel(15);
    int initial_channel = tv_.GetChannel().value();
    
    tv_.SelectChannel(15);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(initial_channel));
}