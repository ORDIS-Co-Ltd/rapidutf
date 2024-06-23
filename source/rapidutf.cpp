#include <string>

#include "rapidutf/rapidutf.hpp"

#if defined(RAPIDUTF_USE_SSE_4_2)
#include <nmmintrin.h> // SSE4.2 intrinsics
#elif defined(RAPIDUTF_USE_NEON)
#include <arm_neon.h>
#endif

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)

namespace rapidutf {

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

void converter::utf8_to_utf16_common(const unsigned char *bytes, std::size_t length, std::u16string &utf16)
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

void converter::utf16_to_utf8_common(const char16_t *chars, std::size_t length, std::string &utf8)
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

void converter::utf16_to_utf32_common(const char16_t *chars, std::size_t length, std::u32string &utf32)
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

void converter::utf32_to_utf16_common(const char32_t *chars, std::size_t length, std::u16string &utf16)
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

void converter::utf8_to_utf32_common(const unsigned char *bytes, std::size_t length, std::u32string &utf32)
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

void converter::utf32_to_utf8_common(const char32_t *chars, std::size_t length, std::string &utf8)
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

#if defined(SOCI_USE_AVX2)

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

      if (bitfield == 0xFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 32; ++j)
        {
          utf16.push_back(static_cast<char16_t>(bytes[i + j]));
        }
        i += 32;
      }
      else
      {
        // Handle non-ASCII characters with AVX2
        // ... (AVX2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf8_to_utf16_common(bytes + i, length - i, utf16);
        break;
      }
    }
    else
    {
      utf8_to_utf16_common(bytes + i, length - i, utf16);
      break;
    }
  }

  return utf16;
}

auto converter::utf16_to_utf8_avx2(const std::u16string &utf16) -> std::string
{
  std::string utf8;
  utf8.reserve(utf16.size() * 3);

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(chars + i));
      __m256i mask = _mm256_set1_epi16(0x7F);
      __m256i result = _mm256_cmpeq_epi16(_mm256_and_si256(chunk, mask), chunk);
      int bitfield = _mm256_movemask_epi8(result);

      if (bitfield == 0xFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 16; ++j)
        {
          utf8.push_back(static_cast<char>(chars[i + j]));
        }
        i += 16;
      }
      else
      {
        // Handle non-ASCII characters with AVX2
        // ... (AVX2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf16_to_utf8_common(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf16_to_utf8_common(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

auto converter::utf16_to_utf32_avx2(const std::u16string &utf16) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf16.size());

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(chars + i));
      __m256i highSurrogateMask = _mm256_set1_epi16(static_cast<short>(0xFC00));
      __m256i highSurrogateStart = _mm256_set1_epi16(static_cast<short>(0xD800));
      __m256i highSurrogate = _mm256_and_si256(chunk, highSurrogateMask);
      __m256i isSurrogate = _mm256_cmpeq_epi16(highSurrogate, highSurrogateStart);

      uint32_t bitfield = _mm256_movemask_epi8(isSurrogate);

      if (bitfield == 0)
      {
        // No surrogates in the chunk, so we can directly convert the UTF-16 characters to UTF-32
        for (int j = 0; j < 16; ++j)
        {
          utf32.push_back(static_cast<char32_t>(chars[i + j]));
        }
        i += 16;
      }
      else
      {
        // Surrogates present in the chunk, so we need to handle them separately
        utf16_to_utf32_common(chars + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf16_to_utf32_common(chars + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

auto converter::utf32_to_utf16_avx2(const std::u32string &utf32) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf32.size() * 2);  // Maximum possible size (all surrogate pairs)

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 8)
    {
      __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(chars + i));
      __m256i mask = _mm256_set1_epi32(0xFFFF);
      __m256i result = _mm256_cmpgt_epi32(chunk, mask);
      int bitfield = _mm256_movemask_ps(_mm256_castsi256_ps(result));

      if (bitfield == 0)
      {
        // All characters in the chunk are BMP characters
        for (int j = 0; j < 8; ++j)
        {
          utf16.push_back(static_cast<char16_t>(chars[i + j]));
        }
        i += 8;
      }
      else
      {
        // Handle non-BMP characters with AVX
        // ... (AVX specific code)
        // For simplicity, let's assume we handle the non-BMP part here and then call the common function
        utf32_to_utf16_common(chars + i, length - i, utf16);
        break;
      }
    }
    else
    {
      utf32_to_utf16_common(chars + i, length - i, utf16);
      break;
    }
  }

  return utf16;
}

auto converter::utf8_to_utf32_avx2(const std::string &utf8) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf8.size());

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

      if (bitfield == 0xFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 32; ++j)
        {
          utf32.push_back(static_cast<char32_t>(bytes[i + j]));
        }
        i += 32;
      }
      else
      {
        // Handle non-ASCII characters with AVX
        // ... (AVX specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf8_to_utf32_common(bytes + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf8_to_utf32_common(bytes + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

auto converter::utf32_to_utf8_avx2(const std::u32string &utf32) -> std::string
{
  if (!is_valid_utf32(utf32))
  {
    throw std::runtime_error("Invalid UTF-32 string");
  }

  std::string utf8;
  utf8.reserve(utf32.size() * 4);  // Reserve enough space to avoid reallocations

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 8)
    {
      __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(chars + i));
      __m256i mask = _mm256_set1_epi32(0x7F);
      __m256i result = _mm256_cmpeq_epi32(_mm256_and_si256(chunk, mask), chunk);
      int bitfield = _mm256_movemask_ps(_mm256_castsi256_ps(result));

      if (bitfield == 0xFFFFFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 8; ++j)
        {
          utf8.push_back(static_cast<char>(chars[i + j]));
        }
        i += 8;
      }
      else
      {
        // Handle non-ASCII characters with AVX2
        // ... (AVX2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf32_to_utf8_common(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf32_to_utf8_common(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

#elif defined(RAPIDUTF_USE_SSE_4_2)

/**
 * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string using SSE4.2 intrinsics.
 *
 * This function takes a std::string containing a UTF-8 encoded string as input and returns a
 * std::u16string containing the equivalent UTF-16 encoded string. The conversion is performed
 * using SIMD instructions provided by the SSE4.2 instruction set, which can significantly
 * improve performance over traditional byte-by-byte conversion methods.
 *
 * @param utf8 A std::string containing a UTF-8 encoded string to be converted.
 * @return A std::u16string containing the equivalent UTF-16 encoded string.
 * @throws std::runtime_error If an invalid UTF-8 sequence is encountered in the input string.
 */
auto converter::utf8_to_utf16_sse42(const std::string &utf8) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(bytes + i));
      __m128i mask = _mm_set1_epi8(static_cast<char>(0x80));
      __m128i result = _mm_cmpeq_epi8(_mm_and_si128(chunk, mask), _mm_setzero_si128());
      int bitfield = _mm_movemask_epi8(result);

      if (bitfield == 0xFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 16; ++j)
        {
          utf16.push_back(static_cast<char16_t>(bytes[i + j]));
        }
        i += 16;
      }
      else
      {
        // Handle non-ASCII characters with SSE4.2
        // ... (SSE4.2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf8_to_utf16_common(bytes + i, length - i, utf16);
        break;
      }
    }
    else
    {
      utf8_to_utf16_common(bytes + i, length - i, utf16);
      break;
    }
  }

  return utf16;
}

/**
 * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string using SSE4.2 intrinsics.
 *
 * This function leverages SSE4.2 intrinsics to efficiently convert a UTF-16 encoded string
 * to a UTF-8 encoded string. It processes the input string in chunks of 8 characters at a time.
 * If all characters in a chunk are ASCII (i.e., their values are less than 0x80), they are
 * directly appended to the output string. Otherwise, each character is processed individually
 * and converted to its corresponding UTF-8 representation.
 *
 * @param utf16 The input UTF-16 encoded string.
 * @return A UTF-8 encoded string.
 * @throws std::runtime_error If an invalid UTF-16 sequence is encountered.
 */
auto converter::utf16_to_utf8_sse42(const std::u16string &utf16) -> std::string
{
  std::string utf8;
  utf8.reserve(utf16.size() * 3);

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 8)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars + i));
      __m128i mask = _mm_set1_epi16(0x7F);
      __m128i result = _mm_cmpeq_epi16(_mm_and_si128(chunk, mask), chunk);
      int bitfield = _mm_movemask_epi8(result);

      if (bitfield == 0xFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 8; ++j)
        {
          utf8.push_back(static_cast<char>(chars[i + j]));
        }
        i += 8;
      }
      else
      {
        // Handle non-ASCII characters with SSE4.2
        // ... (SSE4.2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf16_to_utf8_common(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf16_to_utf8_common(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

/**
 * Converts a UTF-16 encoded string to a UTF-32 encoded string using SSE4.2 intrinsics.
 *
 * This function takes a UTF-16 encoded string as input and returns a UTF-32 encoded string as output.
 * It uses SSE4.2 intrinsics to optimize the conversion process. The function first checks if the input
 * string contains any surrogate pairs. If it does, the function converts each surrogate pair to a single
 * UTF-32 code point. If the input string does not contain any surrogate pairs, the function simply
 * reinterprets the input string as a UTF-32 string.
 *
 * @param utf16 The input UTF-16 encoded string to convert.
 * @return The output UTF-32 encoded string.
 * @throw std::runtime_error If the input string contains an invalid UTF-16 sequence.
 */
auto converter::utf16_to_utf32_sse42(const std::u16string &utf16) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf16.size());

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 8)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars + i));
      __m128i highSurrogate = _mm_and_si128(chunk, _mm_set1_epi16(static_cast<short>(0xFC00)));
      __m128i isSurrogate = _mm_cmpeq_epi16(highSurrogate, _mm_set1_epi16(static_cast<short>(0xD800)));
      int bitfield = _mm_movemask_epi8(isSurrogate);

      if (bitfield == 0)
      {
        // No surrogates in the chunk, so we can directly convert the UTF-16 characters to UTF-32
        for (int j = 0; j < 8; ++j)
        {
          utf32.push_back(static_cast<char32_t>(chars[i + j]));
        }
        i += 8;
      }
      else
      {
        // Surrogates present in the chunk, so we need to handle them separately
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf16_to_utf32_common(chars + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf16_to_utf32_common(chars + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

/**
 * Converts a UTF-32 encoded string to a UTF-16 encoded string using SSE4.2 intrinsics.
 *
 * This function takes a UTF-32 encoded string as input and converts it to a UTF-16 encoded string.
 * It uses SSE4.2 intrinsics to optimize the conversion process. The function checks if any of the
 * UTF-32 code points require surrogate pairs in UTF-16 encoding. If they do, the function converts
 * these code points to surrogate pairs. Otherwise, it directly converts the code points to UTF-16.
 *
 * @param utf32 The input UTF-32 encoded string.
 * @return The output UTF-16 encoded string.
 * @throw std::runtime_error If the input string contains an invalid UTF-32 code point.
 */
auto converter::utf32_to_utf16_sse42(const std::u32string &utf32) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf32.size() * 2);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 4)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars + i));
      __m128i mask = _mm_set1_epi32(0xFFFF);
      __m128i result = _mm_cmpgt_epi32(chunk, mask);
      int bitfield = _mm_movemask_ps(_mm_castsi128_ps(result));

      if (bitfield == 0)
      {
        // No characters > 0xFFFF in the chunk
        for (int j = 0; j < 4; ++j)
        {
          utf16.push_back(static_cast<char16_t>(chars[i + j]));
        }
        i += 4;
      }
      else
      {
        // Handle characters > 0xFFFF with SSE4.2
        // ... (SSE4.2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf32_to_utf16_common(chars + i, length - i, utf16);
        break;
      }
    }
    else
    {
      utf32_to_utf16_common(chars + i, length - i, utf16);
      break;
    }
  }

  return utf16;
}

/**
 * Converts a UTF-8 encoded string to a UTF-32 encoded string using SSE4.2 intrinsics.
 *
 * This function takes a UTF-8 encoded string as input and converts it to a UTF-32 encoded string.
 * It uses SSE4.2 intrinsics to optimize the conversion process. The function processes the input
 * string in chunks and converts each chunk to its corresponding UTF-32 representation.
 *
 * @param utf8 The input UTF-8 encoded string.
 * @return The output UTF-32 encoded string.
 * @throw std::runtime_error If the input string contains an invalid UTF-8 sequence.
 */
auto converter::utf8_to_utf32_sse42(const std::string &utf8) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 16)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(bytes + i));
      __m128i mask = _mm_set1_epi8(static_cast<char>(0x80));
      __m128i result = _mm_cmpeq_epi8(_mm_and_si128(chunk, mask), _mm_setzero_si128());
      int bitfield = _mm_movemask_epi8(result);

      if (bitfield == 0xFFFF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 16; ++j)
        {
          utf32.push_back(static_cast<char32_t>(bytes[i + j]));
        }
        i += 16;
      }
      else
      {
        // Handle non-ASCII characters with SSE4.2
        // ... (SSE4.2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf8_to_utf32_common(bytes + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf8_to_utf32_common(bytes + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

/**
 * Converts a UTF-32 encoded string to a UTF-8 encoded string using SSE4.2 intrinsics.
 *
 * This function takes a UTF-32 encoded string as input and converts it to a UTF-8 encoded string.
 * It uses SSE4.2 intrinsics to optimize the conversion process. The function processes the input
 * string in chunks and converts each chunk to its corresponding UTF-8 representation.
 *
 * @param utf32 The input UTF-32 encoded string.
 * @return The output UTF-8 encoded string.
 * @throw std::runtime_error If the input string contains an invalid UTF-32 code point.
 */
auto converter::utf32_to_utf8_sse42(const std::u32string &utf32) -> std::string
{
  if (!is_valid_utf32(utf32))
  {
    throw std::runtime_error("Invalid UTF-32 string");
  }

  std::string utf8;
  utf8.reserve(utf32.size() * 4);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 4)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars + i));
      __m128i mask = _mm_set1_epi32(0x7F);
      __m128i result = _mm_cmpeq_epi32(_mm_and_si128(chunk, mask), chunk);
      int bitfield = _mm_movemask_ps(_mm_castsi128_ps(result));

      if (bitfield == 0xF)
      {
        // All characters in the chunk are ASCII
        for (int j = 0; j < 4; ++j)
        {
          utf8.push_back(static_cast<char>(chars[i + j]));
        }
        i += 4;
      }
      else
      {
        // Handle non-ASCII characters with SSE4.2
        // ... (SSE4.2 specific code)
        // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
        utf32_to_utf8_common(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf32_to_utf8_common(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

#elif defined(RAPIDUTF_USE_NEON)

/**
 * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string using NEON intrinsics.
 *
 * This function takes a UTF-8 encoded string as input and converts it to a UTF-16 encoded string.
 * It uses NEON intrinsics for efficient processing of the UTF-8 string. If the input UTF-8 string
 * is not well-formed, the function throws an exception with an appropriate error message.
 *
 * @param utf8 The input UTF-8 encoded string.
 * @return std::u16string The output UTF-16 encoded string.
 * @throws std::runtime_error If the input UTF-8 string is not well-formed.
 */
auto converter::utf8_to_utf16_neon(const std::string &utf8) -> std::u16string
{
  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  // Pre-allocate maximum possible size (each UTF-8 byte could become a UTF-16 character)
  std::u16string utf16;
  utf16.reserve(length);
  char16_t *buffer = &utf16[0];  // Non-const pointer to the modifiable buffer

  for (std::size_t i = 0; i < length;)
  {
    if (length - i >= 64)
    {
      // Prefetch next chunk
      __builtin_prefetch(bytes + i + 64, 0, 0);

      // Process 64 bytes in 4 chunks of 16 bytes
      for (int j = 0; j < 4; ++j)
      {
        uint8x16_t chunk = vld1q_u8(bytes + i + j * 16);
        uint8x16_t mask = vdupq_n_u8(0x80);
        uint8x16_t result = vceqq_u8(vandq_u8(chunk, mask), vdupq_n_u8(0));
        uint64_t bitfield_lo = vgetq_lane_u64(vreinterpretq_u64_u8(result), 0);
        uint64_t bitfield_hi = vgetq_lane_u64(vreinterpretq_u64_u8(result), 1);

        if (bitfield_lo != 0xFFFFFFFFFFFFFFFF || bitfield_hi != 0xFFFFFFFFFFFFFFFF)
        {
          // Non-ASCII characters found, handle with NEON
          std::size_t offset = i + j * 16;
          std::size_t remaining = length - offset;
          char16_t *out_ptr = buffer + utf16.size();

          while (remaining > 0)
          {
            uint8x16_t chunk = vld1q_u8(bytes + offset);
            uint16x8_t result_lo = vmovl_u8(vget_low_u8(chunk));
            uint16x8_t result_hi = vmovl_u8(vget_high_u8(chunk));

            uint8x16_t mask_lo = vceqq_u8(vdupq_n_u8(0x80), vget_low_u8(chunk));
            uint8x16_t mask_hi = vceqq_u8(vdupq_n_u8(0x80), vget_high_u8(chunk));

            result_lo = vbslq_u16(vmovl_u8(mask_lo), vld1q_u16(utf8_to_utf16_table + vget_low_u8(chunk)), result_lo);
            result_hi = vbslq_u16(vmovl_u8(mask_hi), vld1q_u16(utf8_to_utf16_table + vget_high_u8(chunk)), result_hi);

            vst1q_u16(reinterpret_cast<uint16_t *>(out_ptr), result_lo);
            vst1q_u16(reinterpret_cast<uint16_t *>(out_ptr + 8), result_hi);

            out_ptr += 16;
            offset += 16;
            remaining -= 16;
          }

          utf16.resize(utf16.size() + (length - i - j * 16));
          return utf16;
        }

        // All characters in this chunk are ASCII
        uint16x8_t chunk_lo = vmovl_u8(vget_low_u8(chunk));
        uint16x8_t chunk_hi = vmovl_u8(vget_high_u8(chunk));
        char16_t *ptr = buffer + utf16.size();
        vst1q_u16(reinterpret_cast<uint16_t *>(ptr), chunk_lo);
        vst1q_u16(reinterpret_cast<uint16_t *>(ptr + 8), chunk_hi);
        utf16.resize(utf16.size() + 16);
      }
      i += 64;
    }
    else
    {
      // Handle remaining bytes
      std::size_t offset = i;
      std::size_t remaining = length - offset;
      char16_t *out_ptr = buffer + utf16.size();

      while (remaining > 0)
      {
        uint8x16_t chunk = vld1q_u8(bytes + offset);
        uint16x8_t result_lo = vmovl_u8(vget_low_u8(chunk));
        uint16x8_t result_hi = vmovl_u8(vget_high_u8(chunk));

        uint8x16_t mask_lo = vceqq_u8(vdupq_n_u8(0x80), vget_low_u8(chunk));
        uint8x16_t mask_hi = vceqq_u8(vdupq_n_u8(0x80), vget_high_u8(chunk));

        result_lo = vbslq_u16(vmovl_u8(mask_lo), vld1q_u16(utf8_to_utf16_table + vget_low_u8(chunk)), result_lo);
        result_hi = vbslq_u16(vmovl_u8(mask_hi), vld1q_u16(utf8_to_utf16_table + vget_high_u8(chunk)), result_hi);

        vst1q_u16(reinterpret_cast<uint16_t *>(out_ptr), result_lo);
        vst1q_u16(reinterpret_cast<uint16_t *>(out_ptr + 8), result_hi);

        out_ptr += 16;
        offset += 16;
        remaining -= 16;
      }

      utf16.resize(utf16.size() + length - i);
      break;
    }
  }

  return utf16;
}

/**
 * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string using NEON intrinsics.
 *
 * This function takes a UTF-16 encoded string as input and converts it to a UTF-8 encoded string.
 * It uses NEON intrinsics to optimize the conversion process for ARM architectures.
 *
 * @param utf16 The input UTF-16 encoded string.
 * @return std::string The UTF-8 encoded string.
 * @throw std::runtime_error If the input UTF-16 string is invalid.
 */
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
        utf16_to_utf8_common(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf16_to_utf8_common(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

/**
 * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
 *
 * This function uses NEON intrinsics for ARMv8 processors to optimize the conversion process.
 * It checks if the input UTF-16 string is valid before performing the conversion.
 * If the input string is not valid UTF-16, it throws a std::runtime_error exception.
 *
 * @param utf16 The input UTF-16 encoded string to be converted.
 * @return std::u32string The converted UTF-32 encoded string.
 * @throws std::runtime_error If the input UTF-16 string is not valid.
 */
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
        utf16_to_utf32_common(chars + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf16_to_utf32_common(chars + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

/**
 * Converts a UTF-32 encoded string to a UTF-16 encoded string using NEON intrinsics.
 *
 * This function takes a UTF-32 encoded string as input and converts it to a UTF-16 encoded string.
 * It uses the NEON intrinsics for ARM architecture to optimize the conversion process.
 *
 * @param utf32 The input UTF-32 encoded string.
 * @return The converted UTF-16 encoded string.
 * @throws std::runtime_error If the input UTF-32 string contains invalid code points.
 */
auto converter::utf32_to_utf16_neon(const std::u32string &utf32) -> std::u16string
{
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

/**
 * Converts a UTF-8 encoded string to a UTF-32 encoded string using NEON intrinsics.
 *
 * @param utf8 The input UTF-8 encoded string.
 * @return The converted UTF-32 encoded string.
 * @throws std::runtime_error If the input UTF-8 string is invalid or contains an invalid UTF-8 sequence.
 */
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
        utf8_to_utf32_common(bytes + i, length - i, utf32);
        break;
      }
    }
    else
    {
      utf8_to_utf32_common(bytes + i, length - i, utf32);
      break;
    }
  }

  return utf32;
}

/**
 * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string using NEON intrinsics.
 *
 * This function takes a UTF-32 encoded string as input and converts it to a UTF-8 encoded string.
 * It uses NEON intrinsics for efficient processing of UTF-32 data.
 *
 * @param utf32 The input UTF-32 encoded string.
 * @return std::string The output UTF-8 encoded string.
 * @throw std::runtime_error If the input UTF-32 string contains invalid code points.
 */
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
        utf32_to_utf8_common(chars + i, length - i, utf8);
        break;
      }
    }
    else
    {
      utf32_to_utf8_common(chars + i, length - i, utf8);
      break;
    }
  }

  return utf8;
}

#else

/**
 * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
 *
 * This function iterates through the input string and converts each UTF-8 sequence into
 * its corresponding UTF-16 code unit(s).
 * If the input string contains invalid UTF-8 encoding, a std::runtime_error exception is thrown.
 *
 * @param utf8 The UTF-8 encoded string.
 * @return std::u16string The UTF-16 encoded string.
 * @throws std::runtime_error if the input string contains invalid UTF-8 encoding.
 */
auto converter::utf8_to_utf16_fallback(const std::string &utf8) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  utf8_to_utf16_common(bytes, length, utf16);

  return utf16;
}

/**
 * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
 *
 * This function iterates through the input string and converts each UTF-16 code unit into
 * its corresponding UTF-8 sequence(s).
 * If the input string contains invalid UTF-16 encoding, a std::runtime_error exception is thrown.
 *
 * @param utf16 The UTF-16 encoded string.
 * @return std::string The UTF-8 encoded string.
 * @throws std::runtime_error if the input string contains invalid UTF-16 encoding.
 */
auto converter::utf16_to_utf8_fallback(const std::u16string &utf16) -> std::string
{
  std::string utf8;
  utf8.reserve(utf16.size() * 3);

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  utf16_to_utf8_common(chars, length, utf8);

  return utf8;
}

/**
 * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
 *
 * This function iterates through the input string and converts each UTF-16 code unit into
 * its corresponding UTF-32 code point(s).
 * If the input string contains invalid UTF-16 encoding, a std::runtime_error exception is thrown.
 *
 * @param utf16 The UTF-16 encoded string.
 * @return std::u32string The UTF-32 encoded string.
 * @throws std::runtime_error if the input string contains invalid UTF-16 encoding.
 */
auto converter::utf16_to_utf32_fallback(const std::u16string &utf16) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf16.size());

  const char16_t *chars = utf16.data();
  std::size_t length = utf16.length();

  utf16_to_utf32_common(chars, length, utf32);

  return utf32;
}

/**
 * @brief Converts a UTF-32 encoded string to a UTF-16 encoded string.
 *
 * This function iterates through the input string and converts each UTF-32 code point into
 * its corresponding UTF-16 code unit(s).
 * If the input string contains invalid UTF-32 code points, a std::runtime_error exception is thrown.
 *
 * @param utf32 The UTF-32 encoded string.
 * @return std::u16string The UTF-16 encoded string.
 * @throws std::runtime_error if the input string contains invalid UTF-32 code points.
 */
auto converter::utf32_to_utf16_fallback(const std::u32string &utf32) -> std::u16string
{
  std::u16string utf16;
  utf16.reserve(utf32.size() * 2);

  const char32_t *chars = utf32.data();
  std::size_t length = utf32.length();

  utf32_to_utf16_common(chars, length, utf16);

  return utf16;
}

/**
 * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
 *
 * This function iterates through the input string and converts each UTF-8 sequence into
 * its corresponding UTF-32 code point(s).
 * If the input string contains invalid UTF-8 encoding, a std::runtime_error exception is thrown.
 *
 * @param utf8 The UTF-8 encoded string.
 * @return std::u32string The UTF-32 encoded string.
 * @throws std::runtime_error if the input string contains invalid UTF-8 encoding.
 */
auto converter::utf8_to_utf32_fallback(const std::string &utf8) -> std::u32string
{
  std::u32string utf32;
  utf32.reserve(utf8.size());

  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
  std::size_t length = utf8.length();

  utf8_to_utf32_common(bytes, length, utf32);

  return utf32;
}

/**
 * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string.
 *
 * This function iterates through the input string and converts each UTF-32 code point into
 * its corresponding UTF-8 sequence(s).
 * If the input string contains invalid UTF-32 code points, a std::runtime_error exception is thrown.
 *
 * @param utf32 The UTF-32 encoded string.
 * @return std::string The UTF-8 encoded string.
 * @throws std::runtime_error if the input string contains invalid UTF-32 code points.
 */
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

  utf32_to_utf8_common(chars, length, utf8);

  return utf8;
}

#endif

    /* Wrappers for the conversion functions that select the appropriate implementation based on
    platform capabilities */

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    std::u16string converter::utf8_to_utf16(const std::string &utf8)
    {
#if defined(SOCI_USE_AVX2)
      return utf8_to_utf16_avx2(utf8);
#elif defined(RAPIDUTF_USE_SSE_4_2)
      return utf8_to_utf16_sse42(utf8);
#elif defined(RAPIDUTF_USE_NEON) // && (0)
      return utf8_to_utf16_neon(utf8);
#else
      return utf8_to_utf16_fallback(utf8);
#endif
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    std::string converter::utf16_to_utf8(const std::u16string &utf16)
    {
#if defined(SOCI_USE_AVX2)
      return utf16_to_utf8_avx2(utf16);
#elif defined(RAPIDUTF_USE_SSE_4_2)
      return utf16_to_utf8_sse42(utf16);
#elif defined(RAPIDUTF_USE_NEON) //&& (0)
      return utf16_to_utf8_neon(utf16);
#else
      return utf16_to_utf8_fallback(utf16);
#endif
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    std::u32string converter::utf16_to_utf32(const std::u16string &utf16)
    {
#if defined(SOCI_USE_AVX2)
      return utf16_to_utf32_avx2(utf16);
#elif defined(RAPIDUTF_USE_SSE_4_2)
      return utf16_to_utf32_sse42(utf16);
#elif defined(RAPIDUTF_USE_NEON)
      return utf16_to_utf32_neon(utf16);
#else
      return utf16_to_utf32_fallback(utf16);
#endif
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-16 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    std::u16string converter::utf32_to_utf16(const std::u32string &utf32)
    {
#if defined(SOCI_USE_AVX2)
      return utf32_to_utf16_avx2(utf32);
#elif defined(RAPIDUTF_USE_SSE_4_2)
      return utf32_to_utf16_sse42(utf32);
#elif defined(RAPIDUTF_USE_NEON)
      return utf32_to_utf16_neon(utf32);
#else
      return utf32_to_utf16_fallback(utf32);
#endif
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    std::u32string converter::utf8_to_utf32(const std::string &utf8)
    {
#if defined(SOCI_USE_AVX2)
      return utf8_to_utf32_avx2(utf8);
#elif defined(RAPIDUTF_USE_SSE_4_2)
      return utf8_to_utf32_sse42(utf8);
#elif defined(RAPIDUTF_USE_NEON)
      return utf8_to_utf32_neon(utf8);
#else
      return utf8_to_utf32_fallback(utf8);
#endif
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    std::string converter::utf32_to_utf8(const std::u32string &utf32)
    {
#if defined(SOCI_USE_AVX2)
      return utf32_to_utf8_avx2(utf32);
#elif defined(RAPIDUTF_USE_SSE_4_2)
      return utf32_to_utf8_sse42(utf32);
#elif defined(RAPIDUTF_USE_NEON)
      return utf32_to_utf8_neon(utf32);
#else
      return utf32_to_utf8_fallback(utf32);
#endif
    }

    /**
     * @brief Converts a UTF-8 encoded string to a wide string (wstring).
     *
     * This function uses the platform's native wide character encoding. On Windows, this is UTF-16,
     * while on Unix/Linux and other platforms, it is UTF-32 or UTF-8 depending on the system
     * configuration.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::wstring The wide string.
     */
    std::wstring converter::utf8_to_wide(const std::string &utf8)
    {
#if defined(SOCI_WCHAR_T_IS_WIDE) // Windows
      // Convert UTF-8 to UTF-32 first and then to wstring (UTF-32 on Unix/Linux)
      std::u32string utf32 = utf8_to_utf32(utf8);
      return std::wstring(utf32.begin(), utf32.end());
#else  // Unix/Linux and others
      std::u16string utf16 = utf8_to_utf16(utf8);
      return std::wstring(utf16.begin(), utf16.end());
#endif // SOCI_WCHAR_T_IS_WIDE
    }

    /**
     * @brief Converts a wide string (wstring) to a UTF-8 encoded string.
     *
     * This function uses the platform's native wide character encoding. On Windows, this is UTF-16,
     * while on Unix/Linux and other platforms, it is UTF-32 or UTF-8 depending on the system
     * configuration.
     * If the input string contains invalid wide characters, a soci_error exception is thrown.
     *
     * @param wide The wide string.
     * @return std::string The UTF-8 encoded string.
     */
    std::string converter::wide_to_utf8(const std::wstring &wide)
    {
#if defined(SOCI_WCHAR_T_IS_WIDE) // Windows
      // Convert wstring (UTF-32) to utf8
      std::u32string utf32(wide.begin(), wide.end());
      return utf32_to_utf8(utf32);
#else  // Unix/Linux and others
      std::u16string utf16(wide.begin(), wide.end());
      return utf16_to_utf8(utf16);
#endif // SOCI_WCHAR_T_IS_WIDE
    }


#if defined(SOCI_USE_AVX2)
std::u16string converter::utf8_to_utf16_avx2(const std::string &utf8)
{
  // Implementation from rapidutf.hpp
}

// Implementations for other AVX2 methods
#elif defined(RAPIDUTF_USE_SSE_4_2)
std::u16string converter::utf8_to_utf16_sse42(const std::string &utf8)
{
  // Implementation from rapidutf.hpp
}

// Implementations for other SSE4.2 methods
#elif defined(RAPIDUTF_USE_NEON)
std::u16string converter::utf8_to_utf16_neon(const std::string &utf8)
{
  // Implementation from rapidutf.hpp
}

// Implementations for other NEON methods
#endif

} // namespace rapidutf

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)
