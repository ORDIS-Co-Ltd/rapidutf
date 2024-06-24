#include <array>
#include <string>

#include "rapidutf/rapidutf.hpp"

#if defined(RAPIDUTF_USE_AVX2)
#  include <immintrin.h>  // SSE4.2 intrinsics
#elif defined(RAPIDUTF_USE_NEON)
#  include <arm_neon.h>
#endif

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)

namespace rapidutf
{

#if defined(_MSC_VER)
// Implementation for MSVC
#  include <intrin.h>
static inline int ctz(uint32_t value)
{
  unsigned long trailing_zero = 0;
  if (_BitScanForward(&trailing_zero, value))
  {
    return static_cast<int>(trailing_zero);
  }
  return 32;
}
#else
// Implementation for GCC and Clang
static inline int ctz(uint32_t value)
{
  return __builtin_ctz(value);
}
#endif

auto converter::is_valid_utf8_sequence(const unsigned char *bytes, int length) -> bool
{
  if (length == 1)
  {
    return (bytes[0] & 0x80) == 0;
  }
  else if (length == 2)
  {
    return (bytes[0] & 0xE0) == 0xC0 && (bytes[1] & 0xC0) == 0x80;
  }
  else if (length == 3)
  {
    return (bytes[0] & 0xF0) == 0xE0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80;
  }
  else if (length == 4)
  {
    return (bytes[0] & 0xF8) == 0xF0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80 && (bytes[3] & 0xC0) == 0x80;
  }
  return false;
}

auto converter::is_valid_utf8(const std::string &utf8) -> bool
{
  const auto *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  for (std::size_t i = 0; i < length;)
  {
    if ((bytes[i] & 0x80) == 0)
    {
      // ASCII character, one byte
      i += 1;
    }
    else if ((bytes[i] & 0xE0) == 0xC0)
    {
      // Two-byte character, check if the next byte is a valid continuation byte
      if (i + 1 >= length || !is_valid_utf8_sequence(bytes + i, 2))
        return false;
      i += 2;
    }
    else if ((bytes[i] & 0xF0) == 0xE0)
    {
      // Three-byte character, check if the next two bytes are valid continuation bytes
      if (i + 2 >= length || !is_valid_utf8_sequence(bytes + i, 3))
        return false;
      i += 3;
    }
    else if ((bytes[i] & 0xF8) == 0xF0)
    {
      // Four-byte character, check if the next three bytes are valid continuation bytes
      if (i + 3 >= length || !is_valid_utf8_sequence(bytes + i, 4))
        return false;
      i += 4;
    }
    else
    {
      // Invalid start byte
      return false;
    }
  }
  return true;
}

auto converter::is_valid_utf16(const std::u16string &utf16) -> bool
{
  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  for (std::size_t i = 0; i < length; ++i)
  {
    char16_t c = chars[i];
    if (c >= 0xD800 && c <= 0xDBFF)
    {
      if (i + 1 >= length)
        return false;
      char16_t next = chars[i + 1];
      if (next < 0xDC00 || next > 0xDFFF)
        return false;
      ++i;
    }
    else if (c >= 0xDC00 && c <= 0xDFFF)
    {
      return false;
    }
  }
  return true;
}

auto converter::is_valid_utf32(const std::u32string &utf32) -> bool
{
  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  for (std::size_t i = 0; i < length; ++i)
  {
    char32_t c = chars[i];

    // Check if the code point is within the Unicode range
    if (c > 0x10FFFF)
    {
      return false;
    }

    // Surrogate pairs are not valid in UTF-32
    if (c >= 0xD800 && c <= 0xDFFF)
    {
      return false;
    }
  }

  return true;
}

void converter::utf8_to_utf16_scalar(const unsigned char *bytes, std::size_t length, std::u16string &utf16)
{
  for (std::size_t i = 0; i < length;)
  {
    if ((bytes[i] & 0x80) == 0)
    {
      // ASCII character, one byte
      utf16.push_back(static_cast<char16_t>(bytes[i]));
      i += 1;
    }
    else if ((bytes[i] & 0xE0) == 0xC0)
    {
      // Two-byte character, check if the next byte is a valid continuation byte
      if (i + 1 >= length || !is_valid_utf8_sequence(bytes + i, 2))
        throw std::runtime_error("Invalid UTF-8 sequence");
      utf16.push_back(static_cast<char16_t>(((bytes[i] & 0x1F) << 6) | (bytes[i + 1] & 0x3F)));
      i += 2;
    }
    else if ((bytes[i] & 0xF0) == 0xE0)
    {
      // Three-byte character, check if the next two bytes are valid continuation bytes
      if (i + 2 >= length || !is_valid_utf8_sequence(bytes + i, 3))
        throw std::runtime_error("Invalid UTF-8 sequence");
      utf16.push_back(static_cast<char16_t>(((bytes[i] & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F)));
      i += 3;
    }
    else if ((bytes[i] & 0xF8) == 0xF0)
    {
      // Four-byte character, check if the next three bytes are valid continuation bytes
      if (i + 3 >= length || !is_valid_utf8_sequence(bytes + i, 4))
        throw std::runtime_error("Invalid UTF-8 sequence");
      uint32_t codepoint = (static_cast<uint32_t>(bytes[i] & 0x07) << 18) | (static_cast<uint32_t>(bytes[i + 1] & 0x3F) << 12) | (static_cast<uint32_t>(bytes[i + 2] & 0x3F) << 6)
        | (static_cast<uint32_t>(bytes[i + 3] & 0x3F));
      if (codepoint > 0x10FFFF)
        throw std::runtime_error("Invalid UTF-8 codepoint");
      if (codepoint <= 0xFFFF)
      {
        utf16.push_back(static_cast<char16_t>(codepoint));
      }
      else
      {
        // Encode as a surrogate pair
        codepoint -= 0x10000;
        utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
        utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
      }
      i += 4;
    }
    else
    {
      throw std::runtime_error("Invalid UTF-8 sequence");
    }
  }
}

void converter::utf16_to_utf8_scalar(const char16_t *chars, std::size_t length, std::string &utf8)
{
  for (std::size_t i = 0; i < length; ++i)
  {
    char16_t c = chars[i];

    if (c < 0x80)
    {
      // 1-byte sequence (ASCII)
      utf8.push_back(static_cast<char>(c));
    }
    else if (c < 0x800)
    {
      // 2-byte sequence
      utf8.push_back(static_cast<char>(0xC0 | ((c >> 6) & 0x1F)));
      utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
    else if ((c >= 0xD800) && (c <= 0xDBFF))
    {
      // Handle UTF-16 surrogate pairs
      if (i + 1 >= length)
        throw std::runtime_error("Invalid UTF-16 sequence");

      char16_t c2 = chars[i + 1];
      if ((c2 < 0xDC00) || (c2 > 0xDFFF))
        throw std::runtime_error("Invalid UTF-16 sequence");

      uint32_t codepoint = static_cast<uint32_t>(((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
      utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
      utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      ++i;  // Skip the next character as it is part of the surrogate pair
    }
    else if ((c >= 0xDC00) && (c <= 0xDFFF))
    {
      // Lone low surrogate, not valid by itself
      throw std::runtime_error("Invalid UTF-16 sequence");
    }
    else
    {
      // 3-byte sequence
      utf8.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
      utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
  }
}

void converter::utf16_to_utf32_scalar(const char16_t *chars, std::size_t length, std::u32string &utf32)
{
  for (std::size_t i = 0; i < length; ++i)
  {
    char16_t c = chars[i];

    if (c >= 0xD800 && c <= 0xDBFF)
    {
      // High surrogate, must be followed by a low surrogate
      if (i + 1 >= length)
        throw std::runtime_error("Invalid UTF-16 sequence (truncated surrogate pair)");

      char16_t c2 = chars[i + 1];
      if (c2 < 0xDC00 || c2 > 0xDFFF)
        throw std::runtime_error("Invalid UTF-16 sequence (invalid surrogate pair)");

      uint32_t codepoint = static_cast<uint32_t>(((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
      utf32.push_back(codepoint);
      ++i;  // Skip the next character as it is part of the surrogate pair
    }
    else if (c >= 0xDC00 && c <= 0xDFFF)
    {
      // Low surrogate without preceding high surrogate
      throw std::runtime_error("Invalid UTF-16 sequence (lone low surrogate)");
    }
    else
    {
      // Valid BMP character
      utf32.push_back(static_cast<char32_t>(c));
    }
  }
}

void converter::utf32_to_utf16_scalar(const char32_t *chars, std::size_t length, std::u16string &utf16)
{
  for (std::size_t i = 0; i < length; ++i)
  {
    char32_t codepoint = chars[i];

    if (codepoint <= 0xFFFF)
    {
      // BMP character
      utf16.push_back(static_cast<char16_t>(codepoint));
    }
    else if (codepoint <= 0x10FFFF)
    {
      // Encode as a surrogate pair
      codepoint -= 0x10000;
      utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
      utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
    }
    else
    {
      // Invalid Unicode range
      throw std::runtime_error("Invalid UTF-32 code point: out of Unicode range");
    }
  }
}

void converter::utf8_to_utf32_scalar(const unsigned char *bytes, std::size_t length, std::u32string &utf32)
{
  for (std::size_t i = 0; i < length;)
  {
    unsigned char c1 = bytes[i];

    if (c1 < 0x80)
    {
      // 1-byte sequence (ASCII)
      utf32.push_back(static_cast<char32_t>(c1));
      ++i;
    }
    else if ((c1 & 0xE0) == 0xC0)
    {
      // 2-byte sequence
      if (i + 1 >= length)
        throw std::runtime_error("Invalid UTF-8 sequence (truncated 2-byte sequence)");

      unsigned char c2 = bytes[i + 1];
      if ((c2 & 0xC0) != 0x80)
        throw std::runtime_error("Invalid UTF-8 sequence (invalid continuation byte in 2-byte sequence)");

      utf32.push_back(static_cast<char32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
      i += 2;
    }
    else if ((c1 & 0xF0) == 0xE0)
    {
      // 3-byte sequence
      if (i + 2 >= length)
        throw std::runtime_error("Invalid UTF-8 sequence (truncated 3-byte sequence)");

      unsigned char c2 = bytes[i + 1];
      unsigned char c3 = bytes[i + 2];
      if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
        throw std::runtime_error("Invalid UTF-8 sequence (invalid continuation bytes in 3-byte sequence)");

      utf32.push_back(static_cast<char32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
      i += 3;
    }
    else if ((c1 & 0xF8) == 0xF0)
    {
      // 4-byte sequence
      if (i + 3 >= length)
        throw std::runtime_error("Invalid UTF-8 sequence (truncated 4-byte sequence)");

      unsigned char c2 = bytes[i + 1];
      unsigned char c3 = bytes[i + 2];
      unsigned char c4 = bytes[i + 3];
      if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
        throw std::runtime_error("Invalid UTF-8 sequence (invalid continuation bytes in 4-byte sequence)");

      utf32.push_back(static_cast<char32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)));
      i += 4;
    }
    else
    {
      throw std::runtime_error("Invalid UTF-8 sequence (invalid start byte)");
    }
  }
}

void converter::utf32_to_utf8_scalar(const char32_t *chars, std::size_t length, std::string &utf8)
{
  for (std::size_t i = 0; i < length; ++i)
  {
    char32_t codepoint = chars[i];

    if (codepoint < 0x80)
    {
      // 1-byte sequence (ASCII)
      utf8.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint < 0x800)
    {
      // 2-byte sequence
      utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
      utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint < 0x10000)
    {
      // 3-byte sequence
      utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
      utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint <= 0x10FFFF)
    {
      // 4-byte sequence
      utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
      utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else
    {
      throw std::runtime_error("Invalid UTF-32 code point");
    }
  }
}

#if defined(RAPIDUTF_USE_AVX2)

auto converter::utf8_to_utf16_avx2(const std::string &utf8) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 32)
    {
      __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(bytes + i));
      __m256i mask = _mm256_set1_epi8(static_cast<char>(0x80));
      __m256i result = _mm256_cmpeq_epi8(_mm256_and_si256(chunk, mask), _mm256_setzero_si256());
      int bitfield = _mm256_movemask_epi8(result);

      if (bitfield == static_cast<int>(0xFFFFFFFF))
      {
        // All characters in the chunk are ASCII
        __m256i ascii_chunk = chunk;

        // Shift left by 8 bits to align ASCII values to high bytes
        __m256i ascii_chunk_hi = _mm256_slli_epi16(ascii_chunk, 8);

        // Combine low and high bytes
        ascii_chunk = _mm256_or_si256(ascii_chunk, ascii_chunk_hi);

        // Store the result directly into the utf16 string
        utf16.resize(utf16.size() + 16);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(&utf16[utf16.size() - 16]), ascii_chunk);

        i += 32;
      }
      else
      {
        // Handle non-ASCII characters with AVX2
        utf8_to_utf16_scalar(bytes + i, length - i, utf16);
        break;
      }
    }
    else
    {
      utf8_to_utf16_scalar(bytes + i, length - i, utf16);
      break;
    }
  }

  return utf16;
}

auto converter::utf16_to_utf8_avx2(const std::u16string &utf16) -> std::string
{
  std::string utf8;
  utf8.reserve(utf16.length() * 3);  // Reserve max possible size

  const char16_t *src = utf16.data();
  size_t len = utf16.length();

  while (len >= 16)
  {
    __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));

    // Check for surrogate pairs
    __m256i surrogates = _mm256_and_si256(_mm256_cmpgt_epi16(input, _mm256_set1_epi16(0xD7FF)), _mm256_cmpgt_epi16(_mm256_set1_epi16(0xE000), input));
    uint32_t surrogate_mask = _mm256_movemask_epi8(surrogates);

    if (surrogate_mask == 0)
    {
      // No surrogate pairs, process 16 characters at once
      __m256i mask1 = _mm256_cmpgt_epi16(input, _mm256_set1_epi16(0x7F));
      __m256i mask2 = _mm256_cmpgt_epi16(input, _mm256_set1_epi16(0x7FF));

      __m256i bytes1 = _mm256_or_si256(_mm256_and_si256(input, _mm256_set1_epi16(0x3F)), _mm256_set1_epi16(0x80));
      __m256i bytes2 = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(input, 6), _mm256_set1_epi16(0x3F)), _mm256_set1_epi16(0x80));
      __m256i bytes3 = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(input, 12), _mm256_set1_epi16(0x0F)), _mm256_set1_epi16(0xE0));

      __m256i result1 = _mm256_blendv_epi8(input, bytes1, mask1);
      __m256i result2 = _mm256_blendv_epi8(bytes2, bytes3, mask2);

      __m256i output = _mm256_packus_epi16(result1, result2);
      output = _mm256_permutevar8x32_epi32(output, _mm256_setr_epi32(0, 1, 4, 5, 2, 3, 6, 7));

      char buffer[32];
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer), output);

      size_t output_size = 16 + _mm256_movemask_epi8(mask1) + _mm256_movemask_epi8(mask2);
      utf8.append(buffer, output_size);

      src += 16;
      len -= 16;
    }
    else
    {
      // Process characters one by one when surrogate pairs are present
      break;
    }
  }

  // Handle remaining characters
  utf16_to_utf8_scalar(utf16.data(), len, utf8);

  return utf8;
}

auto converter::utf16_to_utf32_avx2(const std::u16string &utf16) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf16.size());

  const char16_t *input = utf16.data();
  const size_t length = utf16.size();

  char32_t buffer[16];
  size_t i = 0;

  for (; i + 15 < length; i += 16)
  {
    __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + i));

    // Check for surrogates
    __m256i surr_mask = _mm256_set1_epi16(0xF800);
    __m256i surr_test = _mm256_set1_epi16(0xD800);
    __m256i is_surrogate = _mm256_cmpeq_epi16(_mm256_and_si256(data, surr_mask), surr_test);
    uint32_t mask = _mm256_movemask_epi8(is_surrogate);

    if (mask == 0)
    {
      // No surrogates: we can safely expand directly to UTF-32
      __m256i low = _mm256_unpacklo_epi16(data, _mm256_setzero_si256());
      __m256i high = _mm256_unpackhi_epi16(data, _mm256_setzero_si256());

      _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer), low);
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer + 8), high);
      utf32.insert(utf32.end(), buffer, buffer + 16);
    }
    else
    {
      // Handle surrogates and irregular data with scalar approach
      for (int j = 0; j < 16; ++j)
      {
        char16_t part = input[i + j];
        if (part < 0xD800 || part > 0xDFFF)
        {
          utf32.push_back(part);
        }
        else if (part >= 0xD800 && part <= 0xDBFF)
        {
          if (i + j + 1 >= length)
          {
            throw std::runtime_error("Invalid UTF-16: Unexpected end of input after high surrogate");
          }
          char16_t low_surrogate = input[i + j + 1];
          if (low_surrogate < 0xDC00 || low_surrogate > 0xDFFF)
          {
            throw std::runtime_error("Invalid UTF-16: Invalid low surrogate");
          }
          uint32_t surrogate_pair = 0x10000 + ((part - 0xD800) << 10) + (low_surrogate - 0xDC00);
          utf32.push_back(surrogate_pair);
          ++j;  // skip the next code unit
        }
        else
        {
          throw std::runtime_error("Invalid UTF-16: Unexpected low surrogate");
        }
      }
    }
  }

  // Handle any leftover characters that didn't fit into a 16-character block
  for (; i < length; i++)
  {
    char16_t part = input[i];
    if (part < 0xD800 || part > 0xDFFF)
    {
      utf32.push_back(part);
    }
    else if (part >= 0xD800 && part <= 0xDBFF)
    {
      if (i + 1 >= length)
      {
        throw std::runtime_error("Invalid UTF-16: Unexpected end of input after high surrogate");
      }
      char16_t low_surrogate = input[i + 1];
      if (low_surrogate < 0xDC00 || low_surrogate > 0xDFFF)
      {
        throw std::runtime_error("Invalid UTF-16: Invalid low surrogate");
      }
      uint32_t surrogate_pair = 0x10000 + ((part - 0xD800) << 10) + (low_surrogate - 0xDC00);
      utf32.push_back(surrogate_pair);
      ++i;  // skip the next code unit
    }
    else
    {
      throw std::runtime_error("Invalid UTF-16: Unexpected low surrogate");
    }
  }

  return utf32;
}

auto converter::utf32_to_utf16_avx2(const std::u32string &utf32) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf32.size());

  const uint32_t *src = reinterpret_cast<const uint32_t *>(utf32.data());
  size_t len = utf32.size();

  while (len > 0)
  {
    if (len >= 8)
    {
      __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));

      // Check if all code points are valid (0 <= x <= 0x10FFFF, excluding surrogates)
      __m256i too_large = _mm256_cmpgt_epi32(input, _mm256_set1_epi32(0x10FFFF));
      __m256i surrogate_range = _mm256_and_si256(_mm256_cmpgt_epi32(input, _mm256_set1_epi32(0xD7FF)), _mm256_cmpgt_epi32(_mm256_set1_epi32(0xE000), input));
      __m256i invalid_mask = _mm256_or_si256(too_large, surrogate_range);

      if (_mm256_movemask_ps(_mm256_castsi256_ps(invalid_mask)) != 0)
      {
        throw std::runtime_error("Invalid UTF-32 input");
      }

      // Check if all code points are below 0x10000
      __m256i mask = _mm256_cmpgt_epi32(input, _mm256_set1_epi32(0xFFFF));
      int maskResult = _mm256_movemask_ps(_mm256_castsi256_ps(mask));

      if (maskResult == 0)
      {
        // All code points are below 0x10000, direct conversion
        __m128i low = _mm256_castsi256_si128(input);
        __m128i high = _mm256_extracti128_si256(input, 1);
        __m128i result = _mm_packus_epi32(low, high);

        char16_t buffer[8];
        _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer), result);
        utf16.append(buffer, 8);

        src += 8;
        len -= 8;
      }
      else
      {
        // Some code points are above 0xFFFF, handle individually
        break;
      }
    }
    else
    {
      break;
    }
  }

  // Handle remaining characters
  for (; len > 0; --len, ++src)
  {
    uint32_t codepoint = *src;
    if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
    {
      throw std::runtime_error("Invalid UTF-32 input");
    }
    if (codepoint <= 0xFFFF)
    {
      utf16.push_back(static_cast<char16_t>(codepoint));
    }
    else
    {
      codepoint -= 0x10000;
      utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
      utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
    }
  }

  return utf16;
}

auto converter::utf8_to_utf32_avx2(const std::string &utf8) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf8.size());  // Reserve space for worst case scenario

  const uint8_t *input = reinterpret_cast<const uint8_t *>(utf8.data());
  const uint8_t *end = input + utf8.size();

  __m256i mask_1 = _mm256_set1_epi8(0x80);

  auto throw_invalid_utf8 = []() { throw std::runtime_error("Invalid UTF-8 sequence encountered"); };

  while (input + 32 <= end)
  {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
    uint32_t ascii_mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(chunk, mask_1), _mm256_setzero_si256()));

    if (ascii_mask == 0xFFFFFFFF)
    {
      // All bytes are ASCII
      size_t current_size = utf32.size();
      utf32.resize(current_size + 32);
      char32_t *output = &utf32[current_size];

      _mm_storeu_si128((__m128i *)output, _mm256_castsi256_si128(chunk));
      _mm_storeu_si128((__m128i *)(output + 16), _mm256_extracti128_si256(chunk, 1));

      input += 32;
    }
    else
    {
      // Process ASCII characters in bulk
      int ascii_count = ctz(~ascii_mask);
      if (ascii_count > 0)
      {
        size_t current_size = utf32.size();
        utf32.resize(current_size + ascii_count);
        char32_t *output = &utf32[current_size];

        for (int i = 0; i < ascii_count; ++i)
        {
          output[i] = input[i];
        }
        input += ascii_count;
      }

      // Process the first non-ASCII character
      if (input >= end)
        throw_invalid_utf8();
      if ((*input & 0xE0) == 0xC0)
      {
        if (input + 1 >= end || (input[1] & 0xC0) != 0x80)
          throw_invalid_utf8();
        uint32_t codepoint = ((*input & 0x1F) << 6) | (*(input + 1) & 0x3F);
        if (codepoint < 0x80)
          throw_invalid_utf8();  // Overlong encoding
        utf32.push_back(codepoint);
        input += 2;
      }
      else if ((*input & 0xF0) == 0xE0)
      {
        if (input + 2 >= end || (input[1] & 0xC0) != 0x80 || (input[2] & 0xC0) != 0x80)
          throw_invalid_utf8();
        uint32_t codepoint = ((*input & 0x0F) << 12) | ((*(input + 1) & 0x3F) << 6) | (*(input + 2) & 0x3F);
        if (codepoint < 0x800 || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
          throw_invalid_utf8();  // Overlong encoding or surrogate
        utf32.push_back(codepoint);
        input += 3;
      }
      else if ((*input & 0xF8) == 0xF0)
      {
        if (input + 3 >= end || (input[1] & 0xC0) != 0x80 || (input[2] & 0xC0) != 0x80 || (input[3] & 0xC0) != 0x80)
          throw_invalid_utf8();
        uint32_t codepoint = ((*input & 0x07) << 18) | ((*(input + 1) & 0x3F) << 12) | ((*(input + 2) & 0x3F) << 6) | (*(input + 3) & 0x3F);
        if (codepoint < 0x10000 || codepoint > 0x10FFFF)
          throw_invalid_utf8();  // Overlong encoding or out of Unicode range
        utf32.push_back(codepoint);
        input += 4;
      }
      else
      {
        throw_invalid_utf8();
      }
    }
  }

  // Handle remaining bytes
  while (input < end)
  {
    if (*input < 0x80)
    {
      utf32.push_back(*input++);
    }
    else if ((*input & 0xE0) == 0xC0)
    {
      if (input + 1 >= end || (input[1] & 0xC0) != 0x80)
        throw_invalid_utf8();
      uint32_t codepoint = ((*input & 0x1F) << 6) | (*(input + 1) & 0x3F);
      if (codepoint < 0x80)
        throw_invalid_utf8();  // Overlong encoding
      utf32.push_back(codepoint);
      input += 2;
    }
    else if ((*input & 0xF0) == 0xE0)
    {
      if (input + 2 >= end || (input[1] & 0xC0) != 0x80 || (input[2] & 0xC0) != 0x80)
        throw_invalid_utf8();
      uint32_t codepoint = ((*input & 0x0F) << 12) | ((*(input + 1) & 0x3F) << 6) | (*(input + 2) & 0x3F);
      if (codepoint < 0x800 || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
        throw_invalid_utf8();  // Overlong encoding or surrogate
      utf32.push_back(codepoint);
      input += 3;
    }
    else if ((*input & 0xF8) == 0xF0)
    {
      if (input + 3 >= end || (input[1] & 0xC0) != 0x80 || (input[2] & 0xC0) != 0x80 || (input[3] & 0xC0) != 0x80)
        throw_invalid_utf8();
      uint32_t codepoint = ((*input & 0x07) << 18) | ((*(input + 1) & 0x3F) << 12) | ((*(input + 2) & 0x3F) << 6) | (*(input + 3) & 0x3F);
      if (codepoint < 0x10000 || codepoint > 0x10FFFF)
        throw_invalid_utf8();  // Overlong encoding or out of Unicode range
      utf32.push_back(codepoint);
      input += 4;
    }
    else
    {
      throw_invalid_utf8();
    }
  }

  return utf32;
}

auto converter::utf32_to_utf8_avx2(const std::u32string &utf32) -> std::string
{
  const char32_t *src = utf32.data();
  size_t len = utf32.length();
  std::string utf8;
  utf8.reserve(len * 4);  // Reserve space for worst-case scenario

  size_t i = 0;
  for (; i + 8 <= len; i += 8)
  {
    __m256i codepoints = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + i));

    // Check for invalid codepoints
    __m256i invalid = _mm256_or_si256(_mm256_cmpgt_epi32(codepoints, _mm256_set1_epi32(0x10FFFF)),
                                      _mm256_and_si256(_mm256_cmpgt_epi32(codepoints, _mm256_set1_epi32(0xD7FF)), _mm256_cmpgt_epi32(_mm256_set1_epi32(0xE000), codepoints)));

    if (!_mm256_testz_si256(invalid, invalid))
    {
      throw std::runtime_error("Invalid UTF-32 codepoint detected");
    }

    // Compute the lengths of UTF-8 sequences for each codepoint
    __m256i lengths = _mm256_set1_epi32(1);
    lengths = _mm256_add_epi32(lengths, _mm256_and_si256(_mm256_cmpgt_epi32(codepoints, _mm256_set1_epi32(0x7F)), _mm256_set1_epi32(1)));
    lengths = _mm256_add_epi32(lengths, _mm256_and_si256(_mm256_cmpgt_epi32(codepoints, _mm256_set1_epi32(0x7FF)), _mm256_set1_epi32(1)));
    lengths = _mm256_add_epi32(lengths, _mm256_and_si256(_mm256_cmpgt_epi32(codepoints, _mm256_set1_epi32(0xFFFF)), _mm256_set1_epi32(1)));

    // Compute the total length of the UTF-8 sequences
    __m256i sum = _mm256_hadd_epi32(lengths, lengths);
    sum = _mm256_hadd_epi32(sum, sum);
    int total_length = _mm256_extract_epi32(sum, 0) + _mm256_extract_epi32(sum, 4);

    // Resize the output string to accommodate the new UTF-8 sequences
    size_t prev_size = utf8.size();
    utf8.resize(prev_size + total_length);
    char *dst = &utf8[prev_size];

    // Process each codepoint and generate the corresponding UTF-8 sequence
    for (int j = 0; j < 8; ++j)
    {
      char32_t codepoint = src[i + j];
      if (codepoint <= 0x7F)
      {
        *dst++ = static_cast<char>(codepoint);
      }
      else if (codepoint <= 0x7FF)
      {
        *dst++ = static_cast<char>(0xC0 | (codepoint >> 6));
        *dst++ = static_cast<char>(0x80 | (codepoint & 0x3F));
      }
      else if (codepoint <= 0xFFFF)
      {
        *dst++ = static_cast<char>(0xE0 | (codepoint >> 12));
        *dst++ = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        *dst++ = static_cast<char>(0x80 | (codepoint & 0x3F));
      }
      else
      {
        *dst++ = static_cast<char>(0xF0 | (codepoint >> 18));
        *dst++ = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        *dst++ = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        *dst++ = static_cast<char>(0x80 | (codepoint & 0x3F));
      }
    }
  }

  // Process the remaining codepoints (less than 8)
  for (; i < len; ++i)
  {
    char32_t codepoint = src[i];
    if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
    {
      throw std::runtime_error("Invalid UTF-32 codepoint detected");
    }
    if (codepoint <= 0x7F)
    {
      utf8 += static_cast<char>(codepoint);
    }
    else if (codepoint <= 0x7FF)
    {
      utf8 += static_cast<char>(0xC0 | (codepoint >> 6));
      utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    else if (codepoint <= 0xFFFF)
    {
      utf8 += static_cast<char>(0xE0 | (codepoint >> 12));
      utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
      utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    else
    {
      utf8 += static_cast<char>(0xF0 | (codepoint >> 18));
      utf8 += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
      utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
      utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
  }

  return utf8;
}

#elif defined(RAPIDUTF_USE_NEON)

auto converter::utf8_to_utf16_neon(const std::string &utf8) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      uint8x16_t chunk = vld1q_u8(bytes + i);
      uint8x16_t mask = vdupq_n_u8(0x80);
      uint8x16_t result = vceqq_u8(vandq_u8(chunk, mask), vdupq_n_u8(0));
      uint64_t bitfield_lo = vgetq_lane_u64(vreinterpretq_u64_u8(result), 0);
      uint64_t bitfield_hi = vgetq_lane_u64(vreinterpretq_u64_u8(result), 1);
      uint64_t bitfield = bitfield_lo | (bitfield_hi << 8);

      if (bitfield == 0xFFFFFFFFFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        uint16x8_t chunk_lo = vmovl_u8(vget_low_u8(chunk));
        uint16x8_t chunk_hi = vmovl_u8(vget_high_u8(chunk));
        vst1q_u16(reinterpret_cast<uint16_t *>(&utf16[utf16.size()]), chunk_lo);
        vst1q_u16(reinterpret_cast<uint16_t *>(&utf16[utf16.size() + 8]), chunk_hi);
        utf16.resize(utf16.size() + 16);
        i += 16;
      }
      else
      {
        // Handle non-ASCII characters
        utf8_to_utf16_scalar(bytes + i, length - i, utf16);
        break;
      }
    }
    else
    {
      utf8_to_utf16_scalar(bytes + i, length - i, utf16);
      break;
    }
  }

  return utf16;
}

auto converter::utf16_to_utf8_neon(const std::u16string &utf16) -> std::string
{
  std::string utf8;
  utf8.reserve(utf16.size() * 3);

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      uint16x8_t chunk1 = vld1q_u16(reinterpret_cast<const uint16_t *>(chars + i));
      uint16x8_t chunk2 = vld1q_u16(reinterpret_cast<const uint16_t *>(chars + i + 8));
      uint16x8_t mask = vdupq_n_u16(0x7F);
      uint16x8_t result1 = vceqq_u16(vandq_u16(chunk1, mask), chunk1);
      uint16x8_t result2 = vceqq_u16(vandq_u16(chunk2, mask), chunk2);
      uint64_t bitfield1 = vgetq_lane_u64(vreinterpretq_u64_u16(result1), 0);
      uint64_t bitfield2 = vgetq_lane_u64(vreinterpretq_u64_u16(result2), 0);

      if (bitfield1 == 0xFFFFFFFFFFFFFFFF && bitfield2 == 0xFFFFFFFFFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        uint8x8_t chunk1_lo = vmovn_u16(chunk1);
        uint8x8_t chunk1_hi = vmovn_u16(vshrq_n_u16(chunk1, 8));
        uint8x8_t chunk2_lo = vmovn_u16(chunk2);
        uint8x8_t chunk2_hi = vmovn_u16(vshrq_n_u16(chunk2, 8));

        // Widen the 8-bit vectors to 16-bit vectors
        uint8x16_t chunk1_lo_wide = vcombine_u8(chunk1_lo, chunk1_lo);
        uint8x16_t chunk1_hi_wide = vcombine_u8(chunk1_hi, chunk1_hi);
        uint8x16_t chunk2_lo_wide = vcombine_u8(chunk2_lo, chunk2_lo);
        uint8x16_t chunk2_hi_wide = vcombine_u8(chunk2_hi, chunk2_hi);

        vst1q_u8(reinterpret_cast<uint8_t *>(&utf8[utf8.size()]), chunk1_lo_wide);
        vst1q_u8(reinterpret_cast<uint8_t *>(&utf8[utf8.size() + 16]), chunk1_hi_wide);
        vst1q_u8(reinterpret_cast<uint8_t *>(&utf8[utf8.size() + 32]), chunk2_lo_wide);
        vst1q_u8(reinterpret_cast<uint8_t *>(&utf8[utf8.size() + 48]), chunk2_hi_wide);
        utf8.resize(utf8.size() + 64);
        i += 16;
      }
      else
      {
        // Handle non-ASCII characters with NEON
        utf16_to_utf8_scalar(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf16_to_utf8_scalar(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

auto converter::utf16_to_utf32_neon(const std::u16string &utf16) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf16.size());  // Reserve enough space initially

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  std::size_t i = 0;
  while (i < length)
  {
    if (length - i >= 8)
    {
      uint16x8_t chunk = vld1q_u16(reinterpret_cast<const uint16_t *>(chars + i));
      uint16x8_t highSurrogate = vandq_u16(chunk, vdupq_n_u16(0xFC00));
      uint16x8_t isSurrogate = vceqq_u16(highSurrogate, vdupq_n_u16(0xD800));
      uint64_t bitfield = vgetq_lane_u64(vreinterpretq_u64_u16(isSurrogate), 0);

      if (bitfield == 0)
      {
        // No surrogates in the chunk, so we can directly convert the UTF-16 characters to UTF-32
        uint16x4_t chunk_lo = vget_low_u16(chunk);
        uint16x4_t chunk_hi = vget_high_u16(chunk);
        utf32.resize(utf32.size() + 8);  // Resize once
        utf32[utf32.size() - 8] = static_cast<char32_t>(vget_lane_u16(chunk_lo, 0));
        utf32[utf32.size() - 7] = static_cast<char32_t>(vget_lane_u16(chunk_lo, 1));
        utf32[utf32.size() - 6] = static_cast<char32_t>(vget_lane_u16(chunk_lo, 2));
        utf32[utf32.size() - 5] = static_cast<char32_t>(vget_lane_u16(chunk_lo, 3));
        utf32[utf32.size() - 4] = static_cast<char32_t>(vget_lane_u16(chunk_hi, 0));
        utf32[utf32.size() - 3] = static_cast<char32_t>(vget_lane_u16(chunk_hi, 1));
        utf32[utf32.size() - 2] = static_cast<char32_t>(vget_lane_u16(chunk_hi, 2));
        utf32[utf32.size() - 1] = static_cast<char32_t>(vget_lane_u16(chunk_hi, 3));
        i += 8;
      }
      else
      {
        // Surrogates present in the chunk, so we need to handle them separately
        utf16_to_utf32_scalar(chars + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf16_to_utf32_scalar(chars + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

auto converter::utf32_to_utf16_neon(const std::u32string &utf32) -> std::u16string
{
  if(!is_valid_utf32(utf32))
  {
    throw std::runtime_error("Invalid UTF-32 string");
  }
  
  std::u16string utf16;
  utf16.reserve(utf32.size() * 2);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  if (length >= 16)
  {
    // Process blocks of 16 characters
    std::size_t i = 0;
    for (; i + 16 <= length; i += 16)
    {
      uint32x4_t chunk1 = vld1q_u32(reinterpret_cast<const uint32_t *>(chars + i));
      uint32x4_t chunk2 = vld1q_u32(reinterpret_cast<const uint32_t *>(chars + i + 4));
      uint32x4_t chunk3 = vld1q_u32(reinterpret_cast<const uint32_t *>(chars + i + 8));
      uint32x4_t chunk4 = vld1q_u32(reinterpret_cast<const uint32_t *>(chars + i + 12));

      uint32x4_t mask1 = vcgtq_u32(chunk1, vdupq_n_u32(0xFFFF));
      uint32x4_t mask2 = vcgtq_u32(chunk2, vdupq_n_u32(0xFFFF));
      uint32x4_t mask3 = vcgtq_u32(chunk3, vdupq_n_u32(0xFFFF));
      uint32x4_t mask4 = vcgtq_u32(chunk4, vdupq_n_u32(0xFFFF));

      uint64x2_t result1 = vreinterpretq_u64_u32(mask1);
      uint64x2_t result2 = vreinterpretq_u64_u32(mask2);
      uint64x2_t result3 = vreinterpretq_u64_u32(mask3);
      uint64x2_t result4 = vreinterpretq_u64_u32(mask4);

      uint64_t combined = vgetq_lane_u64(result1, 0) | vgetq_lane_u64(result1, 1) | vgetq_lane_u64(result2, 0) | vgetq_lane_u64(result2, 1) | vgetq_lane_u64(result3, 0)
        | vgetq_lane_u64(result3, 1) | vgetq_lane_u64(result4, 0) | vgetq_lane_u64(result4, 1);

      if (combined == 0)
      {
        // All characters are <= 0xFFFF
        uint16x8_t utf16_chunk1 = vcombine_u16(vmovn_u32(chunk1), vmovn_u32(chunk2));
        uint16x8_t utf16_chunk2 = vcombine_u16(vmovn_u32(chunk3), vmovn_u32(chunk4));

        char16_t temp[16];
        vst1q_u16(reinterpret_cast<uint16_t *>(temp), utf16_chunk1);
        vst1q_u16(reinterpret_cast<uint16_t *>(temp + 8), utf16_chunk2);
        utf16.append(temp, 16);
      }
      else
      {
        // Handle characters > 0xFFFF
        auto process_chunk = [&utf16](uint32x4_t chunk)
        {
          char32_t ch0 = vgetq_lane_u32(chunk, 0);
          char32_t ch1 = vgetq_lane_u32(chunk, 1);
          char32_t ch2 = vgetq_lane_u32(chunk, 2);
          char32_t ch3 = vgetq_lane_u32(chunk, 3);

          for (char32_t ch : {ch0, ch1, ch2, ch3})
          {
            if (ch <= 0xFFFF)
            {
              utf16.push_back(static_cast<char16_t>(ch));
            }
            else
            {
              ch -= 0x10000;
              utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
              utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
            }
          }
        };

        process_chunk(chunk1);
        process_chunk(chunk2);
        process_chunk(chunk3);
        process_chunk(chunk4);
      }
    }

    // Process remaining characters
    for (; i < length; ++i)
    {
      char32_t ch = chars[i];
      if (ch <= 0xFFFF)
      {
        utf16.push_back(static_cast<char16_t>(ch));
      }
      else
      {
        ch -= 0x10000;
        utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
        utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
      }
    }
  }
  else if (length >= 8)
  {
    // Process 8 characters using NEON
    uint32x4_t chunk1 = vld1q_u32(reinterpret_cast<const uint32_t *>(chars));
    uint32x4_t chunk2 = vld1q_u32(reinterpret_cast<const uint32_t *>(chars + 4));

    uint32x4_t mask1 = vcgtq_u32(chunk1, vdupq_n_u32(0xFFFF));
    uint32x4_t mask2 = vcgtq_u32(chunk2, vdupq_n_u32(0xFFFF));

    uint64x2_t result1 = vreinterpretq_u64_u32(mask1);
    uint64x2_t result2 = vreinterpretq_u64_u32(mask2);

    uint64_t combined = vgetq_lane_u64(result1, 0) | vgetq_lane_u64(result1, 1) | vgetq_lane_u64(result2, 0) | vgetq_lane_u64(result2, 1);

    if (combined == 0)
    {
      uint16x8_t utf16_chunk = vcombine_u16(vmovn_u32(chunk1), vmovn_u32(chunk2));
      char16_t temp[8];
      vst1q_u16(reinterpret_cast<uint16_t *>(temp), utf16_chunk);
      utf16.append(temp, 8);
    }
    else
    {
      // Process each character individually
      auto process_chunk = [&utf16](uint32x4_t chunk)
      {
        char32_t ch0 = vgetq_lane_u32(chunk, 0);
        char32_t ch1 = vgetq_lane_u32(chunk, 1);
        char32_t ch2 = vgetq_lane_u32(chunk, 2);
        char32_t ch3 = vgetq_lane_u32(chunk, 3);

        for (char32_t ch : {ch0, ch1, ch2, ch3})
        {
          if (ch <= 0xFFFF)
          {
            utf16.push_back(static_cast<char16_t>(ch));
          }
          else
          {
            ch -= 0x10000;
            utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
            utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
          }
        }
      };

      process_chunk(chunk1);
      process_chunk(chunk2);
    }

    // Process remaining characters
    for (std::size_t i = 8; i < length; ++i)
    {
      char32_t ch = chars[i];
      if (ch <= 0xFFFF)
      {
        utf16.push_back(static_cast<char16_t>(ch));
      }
      else
      {
        ch -= 0x10000;
        utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
        utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
      }
    }
  }
  else if (length >= 4)
  {
    // Process 4 characters using NEON
    uint32x4_t chunk = vld1q_u32(reinterpret_cast<const uint32_t *>(chars));
    uint32x4_t mask = vcgtq_u32(chunk, vdupq_n_u32(0xFFFF));
    uint64x2_t result = vreinterpretq_u64_u32(mask);
    uint64_t combined = vgetq_lane_u64(result, 0) | vgetq_lane_u64(result, 1);

    if (combined == 0)
    {
      uint16x4_t utf16_chunk = vmovn_u32(chunk);
      char16_t temp[4];
      vst1_u16(reinterpret_cast<uint16_t *>(temp), utf16_chunk);
      utf16.append(temp, 4);
    }
    else
    {
      // Process each character individually
      char32_t ch0 = vgetq_lane_u32(chunk, 0);
      char32_t ch1 = vgetq_lane_u32(chunk, 1);
      char32_t ch2 = vgetq_lane_u32(chunk, 2);
      char32_t ch3 = vgetq_lane_u32(chunk, 3);

      for (char32_t ch : {ch0, ch1, ch2, ch3})
      {
        if (ch <= 0xFFFF)
        {
          utf16.push_back(static_cast<char16_t>(ch));
        }
        else
        {
          ch -= 0x10000;
          utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
          utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
        }
      }
    }

    // Process remaining characters
    for (std::size_t i = 4; i < length; ++i)
    {
      char32_t ch = chars[i];
      if (ch <= 0xFFFF)
      {
        utf16.push_back(static_cast<char16_t>(ch));
      }
      else
      {
        ch -= 0x10000;
        utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
        utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
      }
    }
  }
  else
  {
    // Process all characters individually without NEON
    for (std::size_t i = 0; i < length; ++i)
    {
      char32_t ch = chars[i];
      if (ch <= 0xFFFF)
      {
        utf16.push_back(static_cast<char16_t>(ch));
      }
      else
      {
        ch -= 0x10000;
        utf16.push_back(static_cast<char16_t>((ch >> 10) + 0xD800));
        utf16.push_back(static_cast<char16_t>((ch & 0x3FF) + 0xDC00));
      }
    }
  }

  return utf16;
}

auto converter::utf8_to_utf32_neon(const std::string &utf8) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  std::vector<char32_t> buffer(16);  // Temporary buffer to hold 16 char32_t values

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      uint8x16_t chunk = vld1q_u8(bytes + i);
      uint8x16_t mask = vdupq_n_u8(0x80);
      uint8x16_t result = vceqq_u8(vandq_u8(chunk, mask), vdupq_n_u8(0));
      uint64_t bitfield_lo = vgetq_lane_u64(vreinterpretq_u64_u8(result), 0);
      uint64_t bitfield_hi = vgetq_lane_u64(vreinterpretq_u64_u8(result), 1);
      uint64_t bitfield = bitfield_lo | (bitfield_hi << 8);

      if (bitfield == 0xFFFFFFFFFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        uint8x16_t ascii_chars = vld1q_u8(bytes + i);
        uint16x8_t lo_chars = vmovl_u8(vget_low_u8(ascii_chars));
        uint16x8_t hi_chars = vmovl_u8(vget_high_u8(ascii_chars));
        uint32x4_t lo_lo_chars = vmovl_u16(vget_low_u16(lo_chars));
        uint32x4_t lo_hi_chars = vmovl_u16(vget_high_u16(lo_chars));
        uint32x4_t hi_lo_chars = vmovl_u16(vget_low_u16(hi_chars));
        uint32x4_t hi_hi_chars = vmovl_u16(vget_high_u16(hi_chars));

        vst1q_u32(reinterpret_cast<uint32_t *>(buffer.data()), lo_lo_chars);
        vst1q_u32(reinterpret_cast<uint32_t *>(buffer.data()) + 4, lo_hi_chars);
        vst1q_u32(reinterpret_cast<uint32_t *>(buffer.data()) + 8, hi_lo_chars);
        vst1q_u32(reinterpret_cast<uint32_t *>(buffer.data()) + 12, hi_hi_chars);

        utf32.append(buffer.begin(), buffer.end());

        i += 16;
      }
      else
      {
        // Handle non-ASCII characters with NEON
        utf8_to_utf32_scalar(bytes + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf8_to_utf32_scalar(bytes + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

auto converter::utf32_to_utf8_neon(const std::u32string &utf32) -> std::string
{
  if (!is_valid_utf32(utf32))
  {
    throw std::runtime_error("Invalid UTF-32 string");
  }

  std::string utf8;
  utf8.reserve(utf32.size() * 4);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  std::vector<char> buffer(16);  // Temporary buffer to hold 16 UTF-8 bytes

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 4)
    {
      uint32x4_t chunk = vld1q_u32(reinterpret_cast<const uint32_t *>(chars + i));
      uint32x4_t mask = vdupq_n_u32(0x7F);
      uint32x4_t result = vceqq_u32(vandq_u32(chunk, mask), chunk);
      uint64_t bitfield = vgetq_lane_u64(vreinterpretq_u64_u32(result), 0) & 0xFFFFFFFF;

      if (bitfield == 0xFFFFFFFF)
      {
        char buffer[4];
        buffer[0] = static_cast<char>(vgetq_lane_u32(chunk, 0));
        buffer[1] = static_cast<char>(vgetq_lane_u32(chunk, 1));
        buffer[2] = static_cast<char>(vgetq_lane_u32(chunk, 2));
        buffer[3] = static_cast<char>(vgetq_lane_u32(chunk, 3));
        utf8.append(buffer, 4);

        i += 4;
      }
      else
      {
        // Handle non-ASCII characters with NEON
        utf32_to_utf8_scalar(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf32_to_utf8_scalar(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

#else

auto converter::utf8_to_utf16_fallback(const std::string &utf8) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  utf8_to_utf16_scalar(bytes, length, utf16);

  return utf16;
}

auto converter::utf16_to_utf8_fallback(const std::u16string &utf16) -> std::string
{
  std::string utf8;
  utf8.reserve(utf16.size() * 3);

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  utf16_to_utf8_scalar(chars, length, utf8);

  return utf8;
}

auto converter::utf16_to_utf32_fallback(const std::u16string &utf16) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf16.size());

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  utf16_to_utf32_scalar(chars, length, utf32);

  return utf32;
}


auto converter::utf32_to_utf16_fallback(const std::u32string &utf32) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf32.size() * 2);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  utf32_to_utf16_scalar(chars, length, utf16);

  return utf16;
}


auto converter::utf8_to_utf32_fallback(const std::string &utf8) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  utf8_to_utf32_scalar(bytes, length, utf32);

  return utf32;
}

auto converter::utf32_to_utf8_fallback(const std::u32string &utf32) -> std::string
{
  if (!is_valid_utf32(utf32))
  {
    throw std::runtime_error("Invalid UTF-32 string");
  }

  std::string utf8;
  utf8.reserve(utf32.size() * 4);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  utf32_to_utf8_scalar(chars, length, utf8);

  return utf8;
}

#endif

std::u16string converter::utf8_to_utf16(const std::string &utf8)
{
#if defined(RAPIDUTF_USE_AVX2)
  return utf8_to_utf16_avx2(utf8);
#elif defined(RAPIDUTF_USE_SSE_4_2)
  return utf8_to_utf16_sse42(utf8);
#elif defined(RAPIDUTF_USE_NEON)  // && (0)
  return utf8_to_utf16_neon(utf8);
#else
  return utf8_to_utf16_fallback(utf8);
#endif
}

std::string converter::utf16_to_utf8(const std::u16string &utf16)
{
#if defined(RAPIDUTF_USE_AVX2)
  return utf16_to_utf8_avx2(utf16);
#elif defined(RAPIDUTF_USE_SSE_4_2)
  return utf16_to_utf8_sse42(utf16);
#elif defined(RAPIDUTF_USE_NEON)  //&& (0)
  return utf16_to_utf8_neon(utf16);
#else
  return utf16_to_utf8_fallback(utf16);
#endif
}

std::u32string converter::utf16_to_utf32(const std::u16string &utf16)
{
#if defined(RAPIDUTF_USE_AVX2)
  return utf16_to_utf32_avx2(utf16);
#elif defined(RAPIDUTF_USE_SSE_4_2)
  return utf16_to_utf32_sse42(utf16);
#elif defined(RAPIDUTF_USE_NEON)
  return utf16_to_utf32_neon(utf16);
#else
  return utf16_to_utf32_fallback(utf16);
#endif
}

std::u16string converter::utf32_to_utf16(const std::u32string &utf32)
{
#if defined(RAPIDUTF_USE_AVX2)
  return utf32_to_utf16_avx2(utf32);
#elif defined(RAPIDUTF_USE_SSE_4_2)
  return utf32_to_utf16_sse42(utf32);
#elif defined(RAPIDUTF_USE_NEON)
  return utf32_to_utf16_neon(utf32);
#else
  return utf32_to_utf16_fallback(utf32);
#endif
}

std::u32string converter::utf8_to_utf32(const std::string &utf8)
{
#if defined(RAPIDUTF_USE_AVX2)
  return utf8_to_utf32_avx2(utf8);
#elif defined(RAPIDUTF_USE_SSE_4_2)
  return utf8_to_utf32_sse42(utf8);
#elif defined(RAPIDUTF_USE_NEON)
  return utf8_to_utf32_neon(utf8);
#else
  return utf8_to_utf32_fallback(utf8);
#endif
}

std::string converter::utf32_to_utf8(const std::u32string &utf32)
{
#if defined(RAPIDUTF_USE_AVX2)
  return utf32_to_utf8_avx2(utf32);
#elif defined(RAPIDUTF_USE_SSE_4_2)
  return utf32_to_utf8_sse42(utf32);
#elif defined(RAPIDUTF_USE_NEON)
  return utf32_to_utf8_neon(utf32);
#else
  return utf32_to_utf8_fallback(utf32);
#endif
}


std::wstring converter::utf8_to_wide(const std::string &utf8)
{
#if defined(SOCI_WCHAR_T_IS_WIDE)  // Windows
  // Convert UTF-8 to UTF-32 first and then to wstring (UTF-32 on Unix/Linux)
  std::u32string utf32 = utf8_to_utf32(utf8);
  return std::wstring(utf32.begin(), utf32.end());
#else  // Unix/Linux and others
  std::u16string utf16 = utf8_to_utf16(utf8);
  return std::wstring(utf16.begin(), utf16.end());
#endif  // SOCI_WCHAR_T_IS_WIDE
}


std::string converter::wide_to_utf8(const std::wstring &wide)
{
#if defined(SOCI_WCHAR_T_IS_WIDE)  // Windows
  // Convert wstring (UTF-32) to utf8
  std::u32string utf32(wide.begin(), wide.end());
  return utf32_to_utf8(utf32);
#else  // Unix/Linux and others
  std::u16string utf16(wide.begin(), wide.end());
  return utf16_to_utf8(utf16);
#endif  // SOCI_WCHAR_T_IS_WIDE
}

}  // namespace rapidutf

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)
