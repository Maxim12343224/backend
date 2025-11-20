#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    // Пустая строка
    BOOST_TEST(UrlDecode(""sv) == ""s);
    
    // Строка без %-последовательностей
    BOOST_TEST(UrlDecode("HelloWorld"sv) == "HelloWorld"s);
    BOOST_TEST(UrlDecode("abc123-_.~"sv) == "abc123-_.~"s);
    
    // Строка с валидными %-последовательностями
    BOOST_TEST(UrlDecode("Hello%20World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("Hello%2BWorld"sv) == "Hello+World"s);
    BOOST_TEST(UrlDecode("Hello%21World"sv) == "Hello!World"s);
    
    // Строка с %-последовательностями в разном регистре
    BOOST_TEST(UrlDecode("Hello%2bWorld"sv) == "Hello+World"s);
    BOOST_TEST(UrlDecode("Hello%2Bworld"sv) == "Hello+world"s);
    BOOST_TEST(UrlDecode("%41%42%43"sv) == "ABC"s);
    
    // Строка с символом +
    BOOST_TEST(UrlDecode("Hello+World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("Hello+%2B+World"sv) == "Hello + World"s);
    
    // Сложный пример
    BOOST_TEST(UrlDecode("Hello%20World%21%20How%20are%20you%3F"sv) == 
               "Hello World! How are you?"s);
}

BOOST_AUTO_TEST_CASE(UrlDecode_invalid_sequences) {
    using namespace std::literals;

    // Неполные %-последовательности
    BOOST_CHECK_THROW(UrlDecode("Hello%"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("Hello%2"sv), std::invalid_argument);
    
    // Некорректные %-последовательности
    BOOST_CHECK_THROW(UrlDecode("Hello%GH"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("Hello%G1"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("Hello%1G"sv), std::invalid_argument);
    
    // Некорректные символы после %
    BOOST_CHECK_THROW(UrlDecode("Hello%XX"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("Hello%++"sv), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(UrlDecode_reserved_characters) {
    using namespace std::literals;

    // Незакодированные зарезервированные символы должны оставаться как есть
    BOOST_TEST(UrlDecode("!#$&'()*+,/:;=?@[]"sv) == "!#$&'()*+,/:;=?@[]"s);
    
    // Смесь закодированных и незакодированных символов
    BOOST_TEST(UrlDecode("Hello%20World!"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode("test%40example.com?name=John+Doe"sv) == 
               "test@example.com?name=John Doe"s);
}