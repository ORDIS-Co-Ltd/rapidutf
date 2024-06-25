#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include <cwchar>

#if WCHAR_MAX > 0xFFFFu
#  define SOCI_WCHAR_T_IS_WIDE
#endif


// #undef RAPIDUTF_USE_NEON
// #undef RAPIDUTF_USE_AVX2

namespace rapidutf
{

class converter
{
public:
  static auto is_valid_utf8_sequence(const unsigned char *bytes, int length) -> bool;
  static auto is_valid_utf8(const std::string &utf8) -> bool;
  static auto is_valid_utf16(const std::u16string &utf16) -> bool;
  static auto is_valid_utf32(const std::u32string &utf32) -> bool;

  static auto utf8_to_utf16(const std::string &utf8) -> std::u16string;
  static auto utf16_to_utf8(const std::u16string &utf16) -> std::string;
  static auto utf16_to_utf32(const std::u16string &utf16) -> std::u32string;
  static auto utf32_to_utf16(const std::u32string &utf32) -> std::u16string;
  static auto utf8_to_utf32(const std::string &utf8) -> std::u32string;
  static auto utf32_to_utf8(const std::u32string &utf32) -> std::string;

  static auto utf8_to_wide(const std::string &utf8) -> std::wstring;
  static auto wide_to_utf8(const std::wstring &wide) -> std::string;

private:
  static auto utf8_to_utf16_scalar(const unsigned char *bytes, std::size_t length, std::u16string &utf16) -> void;
  static auto utf16_to_utf8_scalar(const char16_t *chars, std::size_t length, std::string &utf8) -> void;
  static auto utf16_to_utf32_scalar(const char16_t *chars, std::size_t length, std::u32string &utf32) -> void;
  static auto utf32_to_utf16_scalar(const char32_t *chars, std::size_t length, std::u16string &utf16) -> void;
  static auto utf8_to_utf32_scalar(const unsigned char *bytes, std::size_t length, std::u32string &utf32) -> void;
  static auto utf32_to_utf8_scalar(const char32_t *chars, std::size_t length, std::string &utf8) -> void;

#if defined(RAPIDUTF_USE_AVX2)
  static auto utf8_to_utf16_avx2(const std::string &utf8) -> std::u16string;
  static auto utf16_to_utf8_avx2(const std::u16string &utf16) -> std::string;
  static auto utf16_to_utf32_avx2(const std::u16string &utf16) -> std::u32string;
  static auto utf32_to_utf16_avx2(const std::u32string &utf32) -> std::u16string;
  static auto utf8_to_utf32_avx2(const std::string &utf8) -> std::u32string;
  static auto utf32_to_utf8_avx2(const std::u32string &utf32) -> std::string;
#elif defined(RAPIDUTF_USE_NEON)
  static auto utf8_to_utf16_neon(const std::string &utf8) -> std::u16string;
  static auto utf16_to_utf8_neon(const std::u16string &utf16) -> std::string;
  static auto utf16_to_utf32_neon(const std::u16string &utf16) -> std::u32string;
  static auto utf32_to_utf16_neon(const std::u32string &utf32) -> std::u16string;
  static auto utf8_to_utf32_neon(const std::string &utf8) -> std::u32string;
  static auto utf32_to_utf8_neon(const std::u32string &utf32) -> std::string;
// #else
#endif
  static auto utf8_to_utf16_fallback(const std::string &utf8) -> std::u16string;
  static auto utf16_to_utf8_fallback(const std::u16string &utf16) -> std::string;
  static auto utf16_to_utf32_fallback(const std::u16string &utf16) -> std::u32string;
  static auto utf32_to_utf16_fallback(const std::u32string &utf32) -> std::u16string;
  static auto utf8_to_utf32_fallback(const std::string &utf8) -> std::u32string;
  static auto utf32_to_utf8_fallback(const std::u32string &utf32) -> std::string;
// #endif
};

}  // namespace rapidutf

#endif  // CONVERTER_HPP
