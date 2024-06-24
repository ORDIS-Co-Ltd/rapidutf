#include <string>

#include "rapidutf/rapidutf.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("UTF-8 validation tests", "[unicode]") {
    using namespace rapidutf;

    // Valid UTF-8 strings
    REQUIRE(converter::is_valid_utf8("Hello, world!")); // valid ASCII
    REQUIRE(converter::is_valid_utf8("")); // Empty string
    REQUIRE(converter::is_valid_utf8(u8"Здравствуй, мир!")); // valid UTF-8
    REQUIRE(converter::is_valid_utf8(u8"こんにちは世界")); // valid UTF-8
    REQUIRE(converter::is_valid_utf8(u8"😀😁😂🤣😃😄😅😆")); // valid UTF-8 with emojis

    // Invalid UTF-8 strings
    REQUIRE(!converter::is_valid_utf8("\x80")); // Invalid single byte
    REQUIRE(!converter::is_valid_utf8("\xC3\x28")); // Invalid two-byte character
    REQUIRE(!converter::is_valid_utf8("\xE2\x82")); // Truncated three-byte character
    REQUIRE(!converter::is_valid_utf8("\xF0\x90\x28")); // Truncated four-byte character
    REQUIRE(!converter::is_valid_utf8("\xF0\x90\x8D\x80\x80")); // Extra byte in four-byte character
}

TEST_CASE("UTF-16 validation tests", "[unicode]") {
    using namespace rapidutf;

    // Valid UTF-16 strings
    REQUIRE(converter::is_valid_utf16(u"Hello, world!")); // valid ASCII
    REQUIRE(converter::is_valid_utf16(u"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE(converter::is_valid_utf16(u"こんにちは世界")); // valid Japanese
    REQUIRE(converter::is_valid_utf16(u"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-16 strings
    std::u16string invalid_utf16 = u"";
    invalid_utf16 += 0xD800; // lone high surrogate
    REQUIRE(!converter::is_valid_utf16(invalid_utf16)); // Invalid UTF-16

    std::u16string valid_utf16 = u"";
    valid_utf16 += 0xD800;
    valid_utf16 += 0xDC00; // valid surrogate pair
    REQUIRE(converter::is_valid_utf16(valid_utf16));

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00; // lone low surrogate
    REQUIRE(!converter::is_valid_utf16(invalid_utf16)); // Invalid UTF-16

    invalid_utf16 = u"";
    invalid_utf16 += 0xD800;
    invalid_utf16 += 0xD800; // two high surrogates in a row
    REQUIRE(!converter::is_valid_utf16(invalid_utf16)); // Invalid UTF-16

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00;
    invalid_utf16 += 0xDC00; // two low surrogates in a row
    REQUIRE(!converter::is_valid_utf16(invalid_utf16)); // Invalid UTF-16
}

TEST_CASE("UTF-32 validation tests", "[unicode]") {
    using namespace rapidutf;

    // Valid UTF-32 strings
    REQUIRE(converter::is_valid_utf32(U"Hello, world!")); // valid ASCII
    REQUIRE(converter::is_valid_utf32(U"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE(converter::is_valid_utf32(U"こんにちは世界")); // valid Japanese
    REQUIRE(converter::is_valid_utf32(U"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-32 strings
    REQUIRE(!converter::is_valid_utf32(U"\x110000")); // Invalid UTF-32 code point
    REQUIRE(!converter::is_valid_utf32(U"\x1FFFFF")); // Invalid range
    REQUIRE(!converter::is_valid_utf32(U"\xFFFFFFFF")); // Invalid range
}

TEST_CASE("UTF-16 to UTF-32 conversion tests", "[unicode]") {
    using namespace rapidutf;

    // Valid conversion tests
    REQUIRE(converter::utf16_to_utf32(u"Hello, world!") == U"Hello, world!");
    REQUIRE(converter::utf16_to_utf32(u"こんにちは世界") == U"こんにちは世界");
    REQUIRE(converter::utf16_to_utf32(u"😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(char16_t(0xD83D)); // high surrogate
    utf16.push_back(char16_t(0xDE00)); // low surrogate
    REQUIRE(converter::utf16_to_utf32(utf16) == U"\U0001F600"); // 😀

    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(converter::utf16_to_utf32(invalid_utf16), std::runtime_error);
}

TEST_CASE("UTF-32 to UTF-16 conversion tests", "[unicode]") {
    using namespace rapidutf;

    // Valid conversion tests
    REQUIRE(converter::utf32_to_utf16(U"Hello, world!") == u"Hello, world!");
    REQUIRE(converter::utf32_to_utf16(U"こんにちは世界") == u"こんにちは世界");
    REQUIRE(converter::utf32_to_utf16(U"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    std::u16string expected_utf16;
    expected_utf16.push_back(0xD83D); // high surrogate
    expected_utf16.push_back(0xDE00); // low surrogate
    REQUIRE(converter::utf32_to_utf16(utf32) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(converter::utf32_to_utf16(invalid_utf32), std::runtime_error);
}

TEST_CASE("UTF-8 to UTF-16 conversion tests", "[unicode]") {
    using namespace rapidutf;

    // Valid conversion tests
    REQUIRE(converter::utf8_to_utf16(u8"Hello, world!") == u"Hello, world!");
    REQUIRE(converter::utf8_to_utf16(u8"こんにちは世界") == u"こんにちは世界");
    REQUIRE(converter::utf8_to_utf16(u8"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::u16string expected_utf16 = u"\xD83D\xDE00";
    REQUIRE(converter::utf8_to_utf16(utf8) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(converter::utf8_to_utf16(invalid_utf8), std::runtime_error);
}

TEST_CASE("UTF-16 to UTF-8 conversion tests", "[unicode]") {
    using namespace rapidutf;

    // Valid conversion tests
    REQUIRE(converter::utf16_to_utf8(u"Hello, world!") == u8"Hello, world!");
    REQUIRE(converter::utf16_to_utf8(u"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(converter::utf16_to_utf8(u"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(0xD83D); // high surrogate
    utf16.push_back(0xDE00); // low surrogate
    REQUIRE(converter::utf16_to_utf8(utf16) == "\xF0\x9F\x98\x80"); // 😀 
  
    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(converter::utf16_to_utf8(invalid_utf16), std::runtime_error);
}

TEST_CASE("UTF-8 to UTF-32 conversion tests", "[unicode]") {
    using namespace rapidutf;

    // Valid conversion tests
    REQUIRE(converter::utf8_to_utf32(u8"Hello, world!") == U"Hello, world!");
    REQUIRE(converter::utf8_to_utf32(u8"こんにちは世界") == U"こんにちは世界");
    REQUIRE(converter::utf8_to_utf32(u8"😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    REQUIRE(converter::utf8_to_utf32(utf8) == U"\U0001F600");

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(converter::utf8_to_utf32(invalid_utf8), std::runtime_error);
}

TEST_CASE("UTF-32 to UTF-8 conversion tests", "[unicode]") {
    using namespace rapidutf;

    // Valid conversion tests
    REQUIRE(converter::utf32_to_utf8(U"Hello, world!") == u8"Hello, world!");
    REQUIRE(converter::utf32_to_utf8(U"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(converter::utf32_to_utf8(U"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    REQUIRE(converter::utf32_to_utf8(utf32) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(converter::utf32_to_utf8(invalid_utf32), std::runtime_error);
}
