#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <wchar.h>

// #define RAPIDUTF_USE_NEON

#if WCHAR_MAX > 0xFFFFu
#define SOCI_WCHAR_T_IS_WIDE
#endif

namespace rapidutf
{

class Converter {
public:
    static auto is_valid_utf8_sequence(const unsigned char* bytes, int length) -> bool;
    static bool is_valid_utf8(const std::string& utf8);
    static bool is_valid_utf16(const std::u16string& utf16);
    static bool is_valid_utf32(const std::u32string& utf32);

    static std::u16string utf8_to_utf16(const std::string& utf8);
    static std::string utf16_to_utf8(const std::u16string& utf16);
    static std::u32string utf16_to_utf32(const std::u16string& utf16);
    static std::u16string utf32_to_utf16(const std::u32string& utf32);
    static std::u32string utf8_to_utf32(const std::string& utf8);
    static std::string utf32_to_utf8(const std::u32string& utf32);

    static std::wstring utf8_to_wide(const std::string& utf8);
    static std::string wide_to_utf8(const std::wstring& wide);

private:
    static void utf8_to_utf16_common(const unsigned char* bytes, std::size_t length, std::u16string& utf16);
    static void utf16_to_utf8_common(const char16_t* chars, std::size_t length, std::string& utf8);
    static void utf16_to_utf32_common(const char16_t* chars, std::size_t length, std::u32string& utf32);
    static void utf32_to_utf16_common(const char32_t* chars, std::size_t length, std::u16string& utf16);
    static void utf8_to_utf32_common(const unsigned char* bytes, std::size_t length, std::u32string& utf32);
    static void utf32_to_utf8_common(const char32_t* chars, std::size_t length, std::string& utf8);

#if defined(SOCI_USE_AVX2)
    static std::u16string utf8_to_utf16_avx2(const std::string& utf8);
    static std::string utf16_to_utf8_avx2(const std::u16string& utf16);
    static std::u32string utf16_to_utf32_avx2(const std::u16string& utf16);
    static std::u16string utf32_to_utf16_avx2(const std::u32string& utf32);
    static std::u32string utf8_to_utf32_avx2(const std::string& utf8);
    static std::string utf32_to_utf8_avx2(const std::u32string& utf32);
#elif defined(RAPIDUTF_USE_SSE_4_2)
    static std::u16string utf8_to_utf16_sse42(const std::string& utf8);
    static std::string utf16_to_utf8_sse42(const std::u16string& utf16);
    static std::u32string utf16_to_utf32_sse42(const std::u16string& utf16);
    static std::u16string utf32_to_utf16_sse42(const std::u32string& utf32);
    static std::u32string utf8_to_utf32_sse42(const std::string& utf8);
    static std::string utf32_to_utf8_sse42(const std::u32string& utf32);
#elif defined(RAPIDUTF_USE_NEON)
    static std::u16string utf8_to_utf16_neon(const std::string& utf8);
    static std::string utf16_to_utf8_neon(const std::u16string& utf16);
    static std::u32string utf16_to_utf32_neon(const std::u16string& utf16);
    static std::u16string utf32_to_utf16_neon(const std::u32string& utf32);
    static std::u32string utf8_to_utf32_neon(const std::string& utf8);
    static std::string utf32_to_utf8_neon(const std::u32string& utf32);
#else
    static std::u16string utf8_to_utf16_fallback(const std::string& utf8);
    static std::string utf16_to_utf8_fallback(const std::u16string& utf16);
    static std::u32string utf16_to_utf32_fallback(const std::u16string& utf16);
    static std::u16string utf32_to_utf16_fallback(const std::u32string& utf32);
    static std::u32string utf8_to_utf32_fallback(const std::string& utf8);
    static std::string utf32_to_utf8_fallback(const std::u32string& utf32);
#endif

};

} // namespace rapidutf

#endif // CONVERTER_HPP
