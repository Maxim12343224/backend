#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, EmptyString) {
    EXPECT_EQ(UrlEncode(""sv), ""s);
}

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode("HelloWorld123"sv), "HelloWorld123"s);
    EXPECT_EQ(UrlEncode("abcxyzABCXYZ0123456789"sv), "abcxyzABCXYZ0123456789"s);
}

TEST(UrlEncodeTestSuite, SafeSpecialCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("-_."sv), "-_."s);
    EXPECT_EQ(UrlEncode("hello-world.test~"sv), "hello-world.test~"s);
}

TEST(UrlEncodeTestSuite, SpaceIsEncodedAsPlus) {
    EXPECT_EQ(UrlEncode("hello world"sv), "hello+world"s);
    EXPECT_EQ(UrlEncode("  "sv), "++"s);
    EXPECT_EQ(UrlEncode("a b c"sv), "a+b+c"s);
}

TEST(UrlEncodeTestSuite, ReservedCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("!#$&'()*+,/:;=?@[]"sv), 
              "%21%23%24%26%27%28%29%2a%2b%2c%2f%3a%3b%3d%3f%40%5b%5d"s);
}

TEST(UrlEncodeTestSuite, ExampleFromTask) {
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);
    EXPECT_EQ(UrlEncode("abc*"sv), "abc%2a"s);
}

TEST(UrlEncodeTestSuite, CharsBelow32AreEncoded) {
    // Тест на символы с кодами меньше 32
    std::string test_str;
    test_str += '\x01'; // код 1
    test_str += '\x1F'; // код 31
    test_str += 'a';    // обычный символ
    
    EXPECT_EQ(UrlEncode(test_str), "%01%1fa"s);
}

TEST(UrlEncodeTestSuite, CharsAbove127AreEncoded) {
    // Тест на символы с кодами >= 128
    std::string test_str;
    test_str += '\x80'; // код 128
    test_str += '\xFF'; // код 255
    test_str += 'a';    // обычный символ
    
    EXPECT_EQ(UrlEncode(test_str), "%80%ffa"s);
}

TEST(UrlEncodeTestSuite, MixedContent) {
    EXPECT_EQ(UrlEncode("Hello World! How are you?"sv), 
              "Hello+World%21+How+are+you%3f"s);
    EXPECT_EQ(UrlEncode("test@example.com"sv), "test%40example.com"s);
    EXPECT_EQ(UrlEncode("price=$100&discount=20%"sv), 
              "price%3d%24100%26discount%3d20%25"s);
}