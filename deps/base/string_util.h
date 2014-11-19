// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.

#ifndef BASE_STRING_UTIL_H_
#define BASE_STRING_UTIL_H_

#include <stdarg.h>   // va_list

#include <string>
#include <vector>
#include <utility>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/port.h"
#include "base/string16.h"
#include "base/string_piece.h"  // For implicit conversions.
#include "base/utf.h"

// Safe standard library wrappers for all platforms.

namespace base {

// C standard-library functions like "strncasecmp" and "snprintf" that aren't
// cross-platform are provided as "base::strncasecmp", and their prototypes
// are listed below.  These functions are then implemented as inline calls
// to the platform-specific equivalents in the platform-specific headers.

// Compares the two strings s1 and s2 without regard to case using
// the current locale; returns 0 if they are equal, 1 if s1 > s2, and -1 if
// s2 > s1 according to a lexicographic comparison.
int StrCaseCmp(const char* s1, const char* s2);

// Compares up to count characters of s1 and s2 without regard to case using
// the current locale; returns 0 if they are equal, 1 if s1 > s2, and -1 if
// s2 > s1 according to a lexicographic comparison.
int StrnCaseCmp(const char* s1, const char* s2, size_t count);

// Same as strncmp but for char16 strings.
int StrnCmp16(const char16* s1, const char16* s2, size_t count);

// Wrapper for vsnprintf that always null-terminates and always returns the
// number of characters that would be in an untruncated formatted
// string, even when truncation occurs.
int VsnPrintf(char* buffer, size_t size, const char* format, va_list arguments)
    PRINTF_FORMAT(3, 0);

// vswprintf always null-terminates, but when truncation occurs, it will either
// return -1 or the number of characters that would be in an untruncated
// formatted string.  The actual return value depends on the underlying
// C library's vswprintf implementation.
int VswPrintf(wchar_t* buffer, size_t size,
              const wchar_t* format, va_list arguments) WPRINTF_FORMAT(3, 0);

// Some of these implementations need to be inlined.

// We separate the declaration from the implementation of this inline
// function just so the PRINTF_FORMAT works.
inline int snprintf(char* buffer, size_t size, const char* format, ...)
    PRINTF_FORMAT(3, 4);
inline int snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}

// We separate the declaration from the implementation of this inline
// function just so the WPRINTF_FORMAT works.
inline int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...)
    WPRINTF_FORMAT(3, 4);
inline int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = VswPrintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}

// BSD-style safe and consistent string copy functions.
// Copies |src| to |dst|, where |dst_size| is the total allocated size of |dst|.
// Copies at most |dst_size|-1 characters, and always NULL terminates |dst|, as
// long as |dst_size| is not 0.  Returns the length of |src| in characters.
// If the return value is >= dst_size, then the output was truncated.
// NOTE: All sizes are in number of characters, NOT in bytes.
size_t strlcpy(char* dst, const char* src, size_t dst_size);
size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size);

// Scan a wprintf format string to determine whether it's portable across a
// variety of systems.  This function only checks that the conversion
// specifiers used by the format string are supported and have the same meaning
// on a variety of systems.  It doesn't check for other errors that might occur
// within a format string.
//
// Nonportable conversion specifiers for wprintf are:
//  - 's' and 'c' without an 'l' length modifier.  %s and %c operate on char
//     data on all systems except Windows, which treat them as wchar_t data.
//     Use %ls and %lc for wchar_t data instead.
//  - 'S' and 'C', which operate on wchar_t data on all systems except Windows,
//     which treat them as char data.  Use %ls and %lc for wchar_t data
//     instead.
//  - 'F', which is not identified by Windows wprintf documentation.
//  - 'D', 'O', and 'U', which are deprecated and not available on all systems.
//     Use %ld, %lo, and %lu instead.
//
// Note that there is no portable conversion specifier for char data when
// working with wprintf.
//
// This function is intended to be called from base::vswprintf.
bool IsWprintfFormatPortable(const wchar_t* format);

// translate unicode ctring to a displayable string
// such as "8764" to "≅"
inline void UniCodeStrToStr(const std::string &ucode, std::string *str) {
  char s[5] = {0};
  try {
    Rune r = atoi(ucode.c_str());
    int iret = runetochar(s, &r);
    s[iret]='\0';
  }
  catch(...) {
    s[0] = '\0';
  }
  str->clear();
  str->append(s);
}

inline int StrCaseCmp(const char* string1, const char* string2) {
  return ::strcasecmp(string1, string2);
}

inline int StrnCaseCmp(const char* string1, const char* string2, size_t count) {
  return ::strncasecmp(string1, string2, count);
}

inline int VsnPrintf(char* buffer, size_t size,
                     const char* format, va_list arguments) {
  return ::vsnprintf(buffer, size, format, arguments);
}

inline int StrnCmp16(const char16* s1, const char16* s2, size_t count) {
#if defined(WCHAR_T_IS_UTF16)
  return ::wcsncmp(s1, s2, count);
#elif defined(WCHAR_T_IS_UTF32)
  return c16memcmp(s1, s2, count);
#endif
}

inline int VswPrintf(wchar_t* buffer, size_t size,
                     const wchar_t* format, va_list arguments) {
  DCHECK(IsWprintfFormatPortable(format));
  return ::vswprintf(buffer, size, format, arguments);
}

}  // namespace base

// These threadsafe functions return references to globally unique empty
// strings.
//
// DO NOT USE THESE AS A GENERAL-PURPOSE SUBSTITUTE FOR DEFAULT CONSTRUCTORS.
// There is only one case where you should use these: functions which need to
// return a string by reference (e.g. as a class member accessor), and don't
// have an empty string to use (e.g. in an error case).  These should not be
// used as initializers, function arguments, or return values for functions
// which return by value or outparam.
const std::string& EmptyString();
const std::wstring& EmptyWString();
const string16& EmptyString16();

extern const wchar_t kWhitespaceWide[];
extern const char16 kWhitespaceUTF16[];
extern const char kWhitespaceASCII[];

extern const char kUtf8ByteOrderMark[];

// Removes characters in remove_chars from anywhere in input.  Returns true if
// any characters were removed.
// NOTE: Safe to use the same variable for both input and output.
bool RemoveChars(const std::wstring& input,
                 const wchar_t remove_chars[],
                 std::wstring* output);
bool RemoveChars(const string16& input,
                 const char16 remove_chars[],
                 string16* output);
bool RemoveChars(const std::string& input,
                 const char remove_chars[],
                 std::string* output);

// Removes characters in trim_chars from the beginning and end of input.
// NOTE: Safe to use the same variable for both input and output.
bool TrimString(const std::wstring& input,
                const wchar_t trim_chars[],
                std::wstring* output);
bool TrimString(const string16& input,
                const char16 trim_chars[],
                string16* output);
bool TrimString(const std::string& input,
                const char trim_chars[],
                std::string* output);

// Truncates a string to the nearest UTF-8 character that will leave
// the string less than or equal to the specified byte size.
void TruncateUTF8ToByteSize(const std::string& input,
                            const size_t byte_size,
                            std::string* output);

// Trims any whitespace from either end of the input string.  Returns where
// whitespace was found.
// The non-wide version has two functions:
// * TrimWhitespaceASCII()
//   This function is for ASCII strings and only looks for ASCII whitespace;
// Please choose the best one according to your usage.
// NOTE: Safe to use the same variable for both input and output.
enum TrimPositions {
  TRIM_NONE     = 0,
  TRIM_LEADING  = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};
TrimPositions TrimWhitespace(const std::wstring& input,
                             TrimPositions positions,
                             std::wstring* output);
TrimPositions TrimWhitespace(const string16& input,
                             TrimPositions positions,
                             string16* output);
TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output);

// Deprecated. This function is only for backward compatibility and calls
// TrimWhitespaceASCII().
TrimPositions TrimWhitespace(const std::string& input,
                             TrimPositions positions,
                             std::string* output);

// Searches  for CR or LF characters.  Removes all contiguous whitespace
// strings that contain them.  This is useful when trying to deal with text
// copied from terminals.
// Returns |text|, with the following three transformations:
// (1) Leading and trailing whitespace is trimmed.
// (2) If |trim_sequences_with_line_breaks| is true, any other whitespace
//     sequences containing a CR or LF are trimmed.
// (3) All other whitespace sequences are converted to single spaces.
std::wstring CollapseWhitespace(const std::wstring& text,
                                bool trim_sequences_with_line_breaks);
string16 CollapseWhitespace(const string16& text,
                            bool trim_sequences_with_line_breaks);
std::string CollapseWhitespaceASCII(const std::string& text,
                                    bool trim_sequences_with_line_breaks);

// Returns true if the passed string is empty or contains only white-space
// characters.
bool ContainsOnlyWhitespaceASCII(const std::string& str);
bool ContainsOnlyWhitespace(const string16& str);

// Returns true if |input| is empty or contains only characters found in
// |characters|.
bool ContainsOnlyChars(const std::wstring& input,
                       const std::wstring& characters);
bool ContainsOnlyChars(const string16& input, const string16& characters);
bool ContainsOnlyChars(const std::string& input, const std::string& characters);

// These convert between ASCII (7-bit) and Wide/UTF16 strings.
std::string WideToASCII(const std::wstring& wide);
std::wstring ASCIIToWide(const base::StringPiece& ascii);
std::string UTF16ToASCII(const string16& utf16);
string16 ASCIIToUTF16(const base::StringPiece& ascii);

// Converts the given wide string to the corresponding Latin1. This will fail
// (return false) if any characters are more than 255.
bool WideToLatin1(const std::wstring& wide, std::string* latin1);

// Returns true if the specified string matches the criteria. How can a wide
// string be 8-bit or UTF8? It contains only characters that are < 256 (in the
// first case) or characters that use only 8-bits and whose 8-bit
// representation looks like a UTF-8 string (the second case).
//
// Note that IsStringUTF8 checks not only if the input is structrually
// valid but also if it doesn't contain any non-character codepoint
// (e.g. U+FFFE). It's done on purpose because all the existing callers want
// to have the maximum 'discriminating' power from other encodings. If
// there's a use case for just checking the structural validity, we have to
// add a new function for that.
bool IsString8Bit(const std::wstring& str);
bool IsStringUTF8(const std::string& str);
bool IsStringASCII(const std::wstring& str);
bool IsStringASCII(const base::StringPiece& str);
bool IsStringASCII(const string16& str);

static const unsigned char kToUpperMap[256] = {
  '\0', 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, '\t',
  '\n', 0x0b, 0x0c, '\r', 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1f,  ' ',  '!',  '"',  '#',  '$',  '%',  '&', '\'',
  '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',  '0',  '1',
  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',
  '<',  '=',  '>',  '?',  '@',  'A',  'B',  'C',  'D',  'E',
  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',
  'Z',  '[', '\\',  ']',  '^',  '_',  '`',  'A',  'B',  'C',
  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',
  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z',  '{',  '|',  '}',  '~', 0x7f, 0x80, 0x81,
  0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
  0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
  0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
  0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
  0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd,
  0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1,
  0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb,
  0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
  0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
  0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static const unsigned char kToLowerMap[256] = {
  '\0', 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, '\t',
  '\n', 0x0b, 0x0c, '\r', 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1f,  ' ',  '!',  '"',  '#',  '$',  '%',  '&', '\'',
  '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',  '0',  '1',
  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',
  '<',  '=',  '>',  '?',  '@',  'a',  'b',  'c',  'd',  'e',
  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',
  'z',  '[', '\\',  ']',  '^',  '_',  '`',  'a',  'b',  'c',
  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',
  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
  'x',  'y',  'z',  '{',  '|',  '}',  '~', 0x7f, 0x80, 0x81,
  0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
  0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
  0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
  0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
  0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd,
  0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1,
  0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb,
  0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
  0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
  0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

inline void StringUpperCopy(char* dest, const char* str, int len) {
  int i;
  uint32_t eax, ebx;
  const uint8_t* ustr = (const uint8_t*) str;
  const int leftover = len % 4;
  const int imax = len / 4;
  const uint32_t* s = reinterpret_cast<const uint32_t*>(str);
  uint32_t* d = reinterpret_cast<uint32_t*>(dest);
  for (i = 0; i != imax; ++i) {
    eax = s[i];
    //  This is based on the algorithm by Paul Hsieh
    //  http://www.azillionmonkeys.com/qed/asmexample.html
    ebx = (0x7f7f7f7fu & eax) + 0x05050505u;
    ebx = (0x7f7f7f7fu & ebx) + 0x1a1a1a1au;
    ebx = ((ebx & ~eax) >> 2) & 0x20202020u;
    *d++ = eax - ebx;
  }

  i = imax*4;
  dest = reinterpret_cast<char*>(d);
  switch (leftover) {
    case 3: *dest++ = static_cast<char>(kToUpperMap[ustr[i++]]);
    case 2: *dest++ = static_cast<char>(kToUpperMap[ustr[i++]]);
    case 1: *dest++ = static_cast<char>(kToUpperMap[ustr[i]]);
    case 0: *dest = '\0';
  }
}

inline void StringLowerCopy(char* dest, const char* str, int len) {
  int i;
  uint32_t eax, ebx;
  const uint8_t* ustr = (const uint8_t*) str;
  const int leftover = len % 4;
  const int imax = len / 4;
  const uint32_t* s = (const uint32_t*) str;
  uint32_t* d = reinterpret_cast<uint32_t*>(dest);
  for (i = 0; i != imax; ++i) {
    eax = s[i];
    //  This is based on the algorithm by Paul Hsieh
    //  http://www.azillionmonkeys.com/qed/asmexample.html
    ebx = (0x7f7f7f7fu & eax) + 0x25252525u;
    ebx = (0x7f7f7f7fu & ebx) + 0x1a1a1a1au;
    ebx = ((ebx & ~eax) >> 2) & 0x20202020u;
    *d++ = eax + ebx;
  }

  i = imax*4;
  dest = reinterpret_cast<char*>(d);
  switch (leftover) {
    case 3: *dest++ = static_cast<char>(kToLowerMap[ustr[i++]]);
    case 2: *dest++ = static_cast<char>(kToLowerMap[ustr[i++]]);
    case 1: *dest++ = static_cast<char>(kToLowerMap[ustr[i]]);
    case 0: *dest = '\0';
  }
}

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToLowerASCII(Char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

template <> inline char ToLowerASCII(char c) {
  return kToLowerMap[static_cast<unsigned char>(c)];
}

// Converts the elements of the given string.  This version uses a pointer to
// clearly differentiate it from the non-pointer variant.
template <class str> inline void StringToLowerASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = ToLowerASCII(*i);
}

template <> inline void StringToLowerASCII(std::string* s) {
  //  NOTE:std::string is copy-on-write, here we add this line to
  //  avoid below case happening:
  //  string a = "Hello";
  //  string b(a);
  //  StringToUpperASCII(&b);
  //  a == "hello"
  if (!s->empty()) {
    //  Write only one member, but copy-on-write happens
    (*s)[0] = (*s)[0];
  } else {
    return;
  }
  StringLowerCopy(const_cast<char*>(s->data()),
                  const_cast<char*>(s->data()), s->size());
}

template <class str> inline str StringToLowerASCII(const str& s) {
  // for std::string and std::wstring
  str output(s);
  StringToLowerASCII(&output);
  return output;
}

template <> inline std::string StringToLowerASCII(const std::string& str) {
  std::string s(str.size(), '\0');
  StringLowerCopy(const_cast<char*>(s.data()), str.data(), str.size());
  return s;
}

template <class str> inline void StringToLowerASCII(const str& in, str* out) {
  // for std::string and std::wstring
  out->assign(in);
  StringToLowerASCII(out);
}

template <> inline void StringToLowerASCII(const std::string& in,
                                           std::string* out) {
  out->resize(in.size());
  StringLowerCopy(const_cast<char*>(out->data()), in.data(), in.size());
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToUpperASCII(Char c) {
  return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}

template <> inline char ToUpperASCII(char c) {
  return kToUpperMap[static_cast<unsigned char>(c)];
}

// Converts the elements of the given string.  This version uses a pointer to
// clearly differentiate it from the non-pointer variant.
template <class str> inline void StringToUpperASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = ToUpperASCII(*i);
}

template <> inline void StringToUpperASCII(std::string* s) {
  //  NOTE:std::string is copy-on-write, here we add this line to
  //  avoid below case happening:
  //  string a = "Hello";
  //  string b(a);
  //  StringToUpperASCII(&b);
  //  a == "hello"
  if (!s->empty()) {
    //  Write only one member, but copy-on-write happens
    (*s)[0] = (*s)[0];
  } else {
    return;
  }
  StringUpperCopy(const_cast<char*>(s->data()),
                  const_cast<char*>(s->data()), s->size());
}

template <class str> inline str StringToUpperASCII(const str& s) {
  // for std::string and std::wstring
  str output(s);
  StringToUpperASCII(&output);
  return output;
}

template <> inline std::string StringToUpperASCII(const std::string& str) {
  std::string s(str.size(), '\0');
  StringUpperCopy(const_cast<char*>(s.data()), str.data(), str.size());
  return s;
}

// Compare the lower-case form of the given string against the given ASCII
// string.  This is useful for doing checking if an input string matches some
// token, and it is optimized to avoid intermediate string copies.  This API is
// borrowed from the equivalent APIs in Mozilla.
bool LowerCaseEqualsASCII(const std::string& a, const char* b);
bool LowerCaseEqualsASCII(const std::wstring& a, const char* b);
bool LowerCaseEqualsASCII(const string16& a, const char* b);

// Same thing, but with string iterators instead.
bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                          std::string::const_iterator a_end,
                          const char* b);
bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                          std::wstring::const_iterator a_end,
                          const char* b);
bool LowerCaseEqualsASCII(string16::const_iterator a_begin,
                          string16::const_iterator a_end,
                          const char* b);
bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b);
bool LowerCaseEqualsASCII(const wchar_t* a_begin,
                          const wchar_t* a_end,
                          const char* b);
bool LowerCaseEqualsASCII(const char16* a_begin,
                          const char16* a_end,
                          const char* b);

// Performs a case-sensitive string compare. The behavior is undefined if both
// strings are not ASCII.
bool EqualsASCII(const string16& a, const base::StringPiece& b);

// Returns true if str starts with search, or false otherwise.
bool StartsWithASCII(const std::string& str,
                     const std::string& search,
                     bool case_sensitive);
bool StartsWith(const std::wstring& str,
                const std::wstring& search,
                bool case_sensitive);
bool StartsWith(const string16& str,
                const string16& search,
                bool case_sensitive);

// Returns true if str ends with search, or false otherwise.
bool EndsWith(const std::string& str,
              const std::string& search,
              bool case_sensitive);
bool EndsWith(const std::wstring& str,
              const std::wstring& search,
              bool case_sensitive);
bool EndsWith(const string16& str,
              const string16& search,
              bool case_sensitive);


// Determines the type of ASCII character, independent of locale (the C
// library versions will change based on locale).
template <typename Char>
inline bool IsAsciiWhitespace(Char c) {
  return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
template <typename Char>
inline bool IsAsciiAlpha(Char c) {
  return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}
template <typename Char>
inline bool IsAsciiDigit(Char c) {
  return c >= '0' && c <= '9';
}

// Returns true if it's a whitespace character.
inline bool IsWhitespace(wchar_t c) {
  return wcschr(kWhitespaceWide, c) != NULL;
}

enum DataUnits {
  DATA_UNITS_BYTE = 0,
  DATA_UNITS_KIBIBYTE,
  DATA_UNITS_MEBIBYTE,
  DATA_UNITS_GIBIBYTE,
};

// Return the unit type that is appropriate for displaying the amount of bytes
// passed in.
DataUnits GetByteDisplayUnits(int64 bytes);

// Return a byte string in human-readable format, displayed in units appropriate
// specified by 'units', with an optional unit suffix.
// Ex: FormatBytes(512, DATA_UNITS_KIBIBYTE, true) => "0.5 KB"
// Ex: FormatBytes(10*1024, DATA_UNITS_MEBIBYTE, false) => "0.1"
std::wstring FormatBytes(int64 bytes, DataUnits units, bool show_units);

// As above, but with "/s" units.
// Ex: FormatSpeed(512, DATA_UNITS_KIBIBYTE, true) => "0.5 KB/s"
// Ex: FormatSpeed(10*1024, DATA_UNITS_MEBIBYTE, false) => "0.1"
std::wstring FormatSpeed(int64 bytes, DataUnits units, bool show_units);

// Return a number formated with separators in the user's locale way.
// Ex: FormatNumber(1234567) => 1,234,567
std::wstring FormatNumber(int64 number);

// Starting at |start_offset| (usually 0), replace the first instance of
// |find_this| with |replace_with|.
void ReplaceFirstSubstringAfterOffset(string16* str,
                                      string16::size_type start_offset,
                                      const string16& find_this,
                                      const string16& replace_with);
void ReplaceFirstSubstringAfterOffset(std::string* str,
                                      std::string::size_type start_offset,
                                      const std::string& find_this,
                                      const std::string& replace_with);

// Starting at |start_offset| (usually 0), look through |str| and replace all
// instances of |find_this| with |replace_with|.
//
// This does entire substrings; use std::replace in <algorithm> for single
// characters, for example:
//   std::replace(str.begin(), str.end(), 'a', 'b');
void ReplaceSubstringsAfterOffset(string16* str,
                                  string16::size_type start_offset,
                                  const string16& find_this,
                                  const string16& replace_with);
void ReplaceSubstringsAfterOffset(std::string* str,
                                  std::string::size_type start_offset,
                                  const std::string& find_this,
                                  const std::string& replace_with);

// Specialized string-conversion functions.
std::string IntToBytes(int value);
std::string Int64ToBytes(int64 value);
int BytesToInt(const char **buffer);
int64 BytesToInt64(const char **buffer);
std::string IntToString(int value);
std::wstring IntToWString(int value);
string16 IntToString16(int value);
std::string UintToString(unsigned int value);
std::wstring UintToWString(unsigned int value);
string16 UintToString16(unsigned int value);
std::string Int64ToString(int64 value);
std::wstring Int64ToWString(int64 value);
std::string Uint64ToString(uint64 value);
std::wstring Uint64ToWString(uint64 value);
// The DoubleToString methods convert the double to a string format that
// ignores the locale.  If you want to use locale specific formatting, use ICU.
std::string DoubleToString(double value);
std::wstring DoubleToWString(double value);

std::string Int64ToHexString(int64 value);
void Int64ToHexString(int64 value, std::string* str);

// Perform a best-effort conversion of the input string to a numeric type,
// setting |*output| to the result of the conversion.  Returns true for
// "perfect" conversions; returns false in the following cases:
//  - Overflow/underflow.  |*output| will be set to the maximum value supported
//    by the data type.
//  - Trailing characters in the string after parsing the number.  |*output|
//    will be set to the value of the number that was parsed.
//  - No characters parseable as a number at the beginning of the string.
//    |*output| will be set to 0.
//  - Empty string.  |*output| will be set to 0.
bool StringToBool(const std::string& intput, bool* output);
bool StringToInt(const std::string& input, int* output);
bool StringToInt(const string16& input, int* output);
bool StringToInt64(const std::string& input, int64* output);
bool StringToInt64(const string16& input, int64* output);
bool StringToUint64(const std::string& input, uint64* output);
bool StringToUint64(const string16& input, uint64* output);
bool SizeStringToUint64(const std::string& input_string, uint64* output);
bool HexStringToInt(const std::string& input, int* output);
bool HexStringToInt(const string16& input, int* output);
bool HexStringToInt64(const std::string& input, int64* output);

// Similar to the previous functions, except that output is a vector of bytes.
// |*output| will contain as many bytes as were successfully parsed prior to the
// error.  There is no overflow, but input.size() must be evenly divisible by 2.
// Leading 0x or +/- are not allowed.
bool HexStringToBytes(const std::string& input, std::vector<uint8>* output);
bool HexStringToBytes(const string16& input, std::vector<uint8>* output);

// For floating-point conversions, only conversions of input strings in decimal
// form are defined to work.  Behavior with strings representing floating-point
// numbers in hexadecimal, and strings representing non-fininte values (such as
// NaN and inf) is undefined.  Otherwise, these behave the same as the integral
// variants.  This expects the input string to NOT be specific to the locale.
// If your input is locale specific, use ICU to read the number.
bool StringToDouble(const std::string& input, double* output);
bool StringToDouble(const string16& input, double* output);

// Convenience forms of the above, when the caller is uninterested in the
// boolean return value.  These return only the |*output| value from the
// above conversions: a best-effort conversion when possible, otherwise, 0.
int StringToInt(const std::string& value);
int StringToInt(const string16& value);
int64 StringToInt64(const std::string& value);
int64 StringToInt64(const string16& value);
int HexStringToInt(const std::string& value);
int HexStringToInt(const string16& value);
int64 HexStringToInt64(const std::string& value);
double StringToDouble(const std::string& value);
double StringToDouble(const string16& value);

// Return a C++ string given printf-like input.
const std::string StringPrintf(const char* format, ...) PRINTF_FORMAT(1, 2);
const std::wstring StringPrintf(const wchar_t* format, ...)
    WPRINTF_FORMAT(1, 2);

// Return a C++ string given vprintf-like input.
std::string StringPrintV(const char* format, va_list ap) PRINTF_FORMAT(1, 0);

// Store result into a supplied string and return it
const std::string& SStringPrintf(std::string* dst, const char* format, ...)
    PRINTF_FORMAT(2, 3);
const std::wstring& SStringPrintf(std::wstring* dst,
                                  const wchar_t* format, ...)
    WPRINTF_FORMAT(2, 3);

// Append result to a supplied string
void StringAppendF(std::string* dst, const char* format, ...)
    PRINTF_FORMAT(2, 3);
void StringAppendF(std::wstring* dst, const wchar_t* format, ...)
    WPRINTF_FORMAT(2, 3);

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
void StringAppendV(std::string* dst, const char* format, va_list ap)
    PRINTF_FORMAT(2, 0);
void StringAppendV(std::wstring* dst, const wchar_t* format, va_list ap)
    WPRINTF_FORMAT(2, 0);

// This is mpcomplete's pattern for saving a string copy when dealing with
// a function that writes results into a wchar_t[] and wanting the result to
// end up in a std::wstring.  It ensures that the std::wstring's internal
// buffer has enough room to store the characters to be written into it, and
// sets its .length() attribute to the right value.
//
// The reserve() call allocates the memory required to hold the string
// plus a terminating null.  This is done because resize() isn't
// guaranteed to reserve space for the null.  The resize() call is
// simply the only way to change the string's 'length' member.
//
// XXX-performance: the call to wide.resize() takes linear time, since it fills
// the string's buffer with nulls.  I call it to change the length of the
// string (needed because writing directly to the buffer doesn't do this).
// Perhaps there's a constant-time way to change the string's length.
template <class string_type>
inline typename string_type::value_type* WriteInto(string_type* str,
                                                   size_t length_with_null) {
  str->reserve(length_with_null);
  str->resize(length_with_null - 1);
  return &((*str)[0]);
}

//-----------------------------------------------------------------------------

// Function objects to aid in comparing/searching strings.

template<typename Char> struct CaseInsensitiveCompare {
 public:
  bool operator()(Char x, Char y) const {
    // TODO(darin): Do we really want to do locale sensitive comparisons here?
    // See http://crbug.com/24917
    return tolower(x) == tolower(y);
  }
};

template<typename Char> struct CaseInsensitiveCompareASCII {
 public:
  bool operator()(Char x, Char y) const {
    return ToLowerASCII(x) == ToLowerASCII(y);
  }
};

// TODO(timsteele): Move these split string functions into their own API on
// string_split.cc/.h files.
//-----------------------------------------------------------------------------

// Splits |str| into a vector of strings delimited by |s|. Append the results
// into |r| as they appear. If several instances of |s| are contiguous, or if
// |str| begins with or ends with |s|, then an empty string is inserted.
//
// Every substring is trimmed of any leading or trailing white space.
void SplitString(const std::wstring& str,
                 wchar_t s,
                 std::vector<std::wstring>* r);
void SplitString(const string16& str,
                 char16 s,
                 std::vector<string16>* r);
void SplitString(const std::string& str,
                 char s,
                 std::vector<std::string>* r);

void SplitString(const base::StringPiece& str,
                 char s,
                 std::vector<base::StringPiece>* r);

// The same as SplitString, but don't trim white space.
void SplitStringDontTrim(const std::wstring& str,
                         wchar_t s,
                         std::vector<std::wstring>* r);
void SplitStringDontTrim(const string16& str,
                         char16 s,
                         std::vector<string16>* r);
void SplitStringDontTrim(const std::string& str,
                         char s,
                         std::vector<std::string>* r);

// The same as SplitString, but use a substring delimiter instead of a char.
void SplitStringUsingSubstr(const string16& str,
                            const string16& s,
                            std::vector<string16>* r);
void SplitStringUsingSubstr(const std::string& str,
                            const std::string& s,
                            std::vector<std::string>* r);

void SplitStringUsingSubstrDontTrim(const std::string& str,
                                    const std::string& s,
                                    std::vector<std::string>* r);

// output is pair of start, len
void SplitStringUsingSubstr(const string16& str,
                            const string16& s,
                            std::vector<std::pair<int, int> >* r);
void SplitStringUsingSubstr(const std::string& str,
                            const std::string& s,
                            std::vector<std::pair<int, int> >* r);

// Splits a string into its fields delimited by any of the characters in
// |delimiters|.  Each field is added to the |tokens| vector.  Returns the
// number of tokens found.
size_t Tokenize(const std::wstring& str,
                const std::wstring& delimiters,
                std::vector<std::wstring>* tokens);
size_t Tokenize(const string16& str,
                const string16& delimiters,
                std::vector<string16>* tokens);
size_t Tokenize(const std::string& str,
                const std::string& delimiters,
                std::vector<std::string>* tokens);
size_t Tokenize(const base::StringPiece& str,
                const base::StringPiece& delimiters,
                std::vector<base::StringPiece>* tokens);

// Does the opposite of SplitString().
std::wstring JoinString(const std::vector<std::wstring>& parts, wchar_t s);
string16 JoinString(const std::vector<string16>& parts, char16 s);
std::string JoinString(const std::vector<std::string>& parts, char s);
std::string JoinString(const std::vector<std::string>& parts,
                       const std::string &s);

// Join array to string
template<typename T>
std::string JoinVector(const std::vector<T>& parts, char sep) {
  if (parts.size() == 0) return "";
  std::stringstream s;
  typename std::vector<T>::const_iterator iter = parts.begin();
  s << *iter++;
  for (; iter != parts.end(); ++iter) {
    s << sep << *iter;
  }
  return s.str();
}

template <typename T>
std::string JoinKeys(T *v, const std::string &sep) {
  if (!v || v->empty()) return "";
  std::stringstream s;
  typename T::const_iterator iter = v->begin();
  s << iter->first;
  for (++iter; iter != v->end(); ++iter) {
    s << sep << iter->first;
  }
  return s.str();
}

template <typename T>
std::string JoinValues(T *v, const std::string &sep) {
  if (!v || v->empty()) return "";
  std::stringstream s;
  typename T::const_iterator iter = v->begin();
  s << iter->second;
  for (++iter; iter != v->end(); ++iter) {
    s << sep << iter->second;
  }
  return s.str();
}


// WARNING: this uses whitespace as defined by the HTML5 spec. If you need
// a function similar to this but want to trim all types of whitespace, then
// factor this out into a function that takes a string containing the characters
// that are treated as whitespace.
//
// Splits the string along whitespace (where whitespace is the five space
// characters defined by HTML 5). Each contiguous block of non-whitespace
// characters is added to result.
void SplitStringAlongWhitespace(const std::wstring& str,
                                std::vector<std::wstring>* result);
void SplitStringAlongWhitespace(const string16& str,
                                std::vector<string16>* result);
void SplitStringAlongWhitespace(const std::string& str,
                                std::vector<std::string>* result);

// Replace $1-$2-$3..$9 in the format string with |a|-|b|-|c|..|i| respectively.
// Additionally, $$ is replaced by $. The offsets parameter here can
// be NULL. This only allows you to use up to nine replacements.
string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const std::vector<string16>& subst,
                                   std::vector<size_t>* offsets);

std::string ReplaceStringPlaceholders(const base::StringPiece& format_string,
                                      const std::vector<std::string>& subst,
                                      std::vector<size_t>* offsets);

// Single-string shortcut for ReplaceStringHolders.
string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   size_t* offset);

// If the size of |input| is more than |max_len|, this function returns true and
// |input| is shortened into |output| by removing chars in the middle (they are
// replaced with up to 3 dots, as size permits).
// Ex: ElideString(L"Hello", 10, &str) puts Hello in str and returns false.
// ElideString(L"Hello my name is Tom", 10, &str) puts "Hell...Tom" in str and
// returns true.
bool ElideString(const std::wstring& input, int max_len, std::wstring* output);

// Returns true if the string passed in matches the pattern. The pattern
// string can contain wildcards like * and ?
// The backslash character (\) is an escape character for * and ?
// We limit the patterns to having a max of 16 * or ? characters.
bool MatchPatternWide(const std::wstring& string, const std::wstring& pattern);
bool MatchPatternASCII(const std::string& string, const std::string& pattern);

// Returns a hex string representation of a binary buffer.
// The returned hex string will be in upper case.
// This function does not check if |size| is within reasonable limits since
// it's written with trusted data in mind.
// If you suspect that the data you want to format might be large,
// the absolute max size for |size| should be is
//   std::numeric_limits<size_t>::max() / 2
std::string HexEncode(const void* bytes, size_t size);

// Hack to convert any char-like type to its unsigned counterpart.
// For example, it will convert char, signed char and unsigned char to unsigned
// char.
template<typename T>
struct ToUnsigned {
  typedef T Unsigned;
};

template<>
struct ToUnsigned<char> {
  typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<signed char> {
  typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<wchar_t> {
#if defined(WCHAR_T_IS_UTF16)
  typedef uint16 Unsigned;
#elif defined(WCHAR_T_IS_UTF32)
  typedef uint32 Unsigned;
#endif
};
template<>
struct ToUnsigned<int16> {
  typedef uint16 Unsigned;
};

#endif  // BASE_STRING_UTIL_H_
