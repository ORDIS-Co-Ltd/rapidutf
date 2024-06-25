#include <string>

#include "rapidutf/rapidutf.hpp"

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN

TEST_CASE("UTF-8 validation tests", "[unicode]") {
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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
    using rapidutf::converter;

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

TEST_CASE("Edge case UTF-8 validation tests", "[unicode]") {
    using rapidutf::converter;

    REQUIRE(converter::is_valid_utf8("\xC2\x80")); // Smallest two-byte character
    REQUIRE(converter::is_valid_utf8("\xDF\xBF")); // Largest two-byte character
    REQUIRE(converter::is_valid_utf8("\xE0\xA0\x80")); // Smallest three-byte character
    REQUIRE(converter::is_valid_utf8("\xEF\xBF\xBF")); // Largest three-byte character
    REQUIRE(converter::is_valid_utf8("\xF0\x90\x80\x80")); // Smallest four-byte character
    REQUIRE(converter::is_valid_utf8("\xF4\x8F\xBF\xBF")); // Largest valid four-byte character

    REQUIRE(!converter::is_valid_utf8("\xF4\x90\x80\x80")); // Just above largest valid four-byte character
    REQUIRE(!converter::is_valid_utf8("\xFE")); // Invalid start byte
    REQUIRE(!converter::is_valid_utf8("\xFF")); // Invalid start byte
    REQUIRE(!converter::is_valid_utf8("\xC0\xAF")); // Overlong encoding
    REQUIRE(!converter::is_valid_utf8("\xE0\x80\xAF")); // Overlong encoding
    REQUIRE(!converter::is_valid_utf8("\xF0\x80\x80\xAF")); // Overlong encoding
}

TEST_CASE("UTF-16 surrogate pair tests", "[unicode]") {
    using rapidutf::converter;

    std::u16string valid_surrogate_pair = u"\xD800\xDC00";
    REQUIRE(converter::is_valid_utf16(valid_surrogate_pair));

    std::u16string invalid_surrogate_order = u"\xDC00\xD800";
    REQUIRE(!converter::is_valid_utf16(invalid_surrogate_order));

}

TEST_CASE("UTF-32 edge case tests", "[unicode]") {
    using rapidutf::converter;

    REQUIRE(converter::is_valid_utf32(U"\x0000")); // Null character
    REQUIRE(converter::is_valid_utf32(U"\x0001")); // Start of valid range
    REQUIRE(converter::is_valid_utf32(U"\xD7FF")); // Last code point before surrogate range
    REQUIRE(converter::is_valid_utf32(U"\xE000")); // First code point after surrogate range
    REQUIRE(converter::is_valid_utf32(U"\x10FFFF")); // Last valid code point

    REQUIRE(!converter::is_valid_utf32(U"\xD800")); // Start of surrogate range
    REQUIRE(!converter::is_valid_utf32(U"\xDFFF")); // End of surrogate range
    REQUIRE(!converter::is_valid_utf32(U"\x110000")); // First invalid code point
}

TEST_CASE("Roundtrip conversion tests", "[unicode]") {
    using rapidutf::converter;

    std::string utf8 = u8"Hello, 世界! 🌍";
    std::u16string utf16 = converter::utf8_to_utf16(utf8);
    std::u32string utf32 = converter::utf16_to_utf32(utf16);

    REQUIRE(converter::utf32_to_utf16(utf32) == utf16);
    REQUIRE(converter::utf16_to_utf8(utf16) == utf8);
    REQUIRE(converter::utf32_to_utf8(utf32) == utf8);
}

TEST_CASE("Empty string conversion tests", "[unicode]") {
    using rapidutf::converter;

    REQUIRE(converter::utf8_to_utf16("") == u"");
    REQUIRE(converter::utf16_to_utf8(u"") == "");
    REQUIRE(converter::utf8_to_utf32("") == U"");
    REQUIRE(converter::utf32_to_utf8(U"") == "");
    REQUIRE(converter::utf16_to_utf32(u"") == U"");
    REQUIRE(converter::utf32_to_utf16(U"") == u"");
}

TEST_CASE("Mixed script conversion tests", "[unicode]") {
    using rapidutf::converter;

    std::string mixed_utf8 = u8"Hello, Здравствуй, こんにちは, 你好, مرحبا, שלום";
    std::u16string mixed_utf16 = converter::utf8_to_utf16(mixed_utf8);
    std::u32string mixed_utf32 = converter::utf8_to_utf32(mixed_utf8);

    REQUIRE(converter::utf16_to_utf8(mixed_utf16) == mixed_utf8);
    REQUIRE(converter::utf32_to_utf8(mixed_utf32) == mixed_utf8);
}

#include <iostream>

TEST_CASE("Mixed script conversion tests (DEBUG)", "[unicode]") {
    using rapidutf::converter;

    std::string mixed_utf8 = u8"Hello, Здравствуй, こんにちは, 你好, مرحبا, שלום";
    std::u16string mixed_utf16 = converter::utf8_to_utf16(mixed_utf8);

    // Print character values of mixed_utf8
    std::cout << "mixed_utf8 character values:" << std::endl;
    for (char c : mixed_utf8) {
        std::cout << static_cast<int>(static_cast<unsigned char>(c)) << " ";
    }
    std::cout << std::endl;

    // Print character values of mixed_utf16
    std::cout << "mixed_utf16 character values:" << std::endl;
    for (char16_t c : mixed_utf16) {
        std::cout << static_cast<int>(c) << " ";
    }
    std::cout << std::endl;

    std::string new_utf8 = converter::utf16_to_utf8(mixed_utf16);

    // Print character values of new_utf8
    std::cout << "new_utf8 character values:" << std::endl;
    for (char c : new_utf8) {
        std::cout << static_cast<int>(static_cast<unsigned char>(c)) << " ";
    }
    std::cout << std::endl;

    // Compare lengths
    REQUIRE(new_utf8.length() == mixed_utf8.length());

    // Compare individual characters
    for (size_t i = 0; i < mixed_utf8.length(); ++i) {
        REQUIRE(new_utf8[i] == mixed_utf8[i]);
    }

    REQUIRE(new_utf8 == mixed_utf8);
}


TEST_CASE("Long string conversion tests", "[unicode]") {
    using rapidutf::converter;

    std::string long_utf8;
    for (int i = 0; i < 1000; ++i) {
        long_utf8 += u8"🌍";
    }

    std::u16string long_utf16 = converter::utf8_to_utf16(long_utf8);
    std::u32string long_utf32 = converter::utf8_to_utf32(long_utf8);

    REQUIRE(converter::utf16_to_utf8(long_utf16) == long_utf8);
    REQUIRE(converter::utf32_to_utf8(long_utf32) == long_utf8);
}

// NOLINTEND