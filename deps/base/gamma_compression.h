// Copyright 2013 Jike Inc. All Rights Reserved.
// Author: liyangguang@jike.com (Yangguang Li)
// File: gamma_compression.h
// Description:
// Created Date: 2013-01-14 19:33:47

#ifndef BASE_GAMMA_COMPRESSION_H_
#define BASE_GAMMA_COMPRESSION_H_

#include "base/basictypes.h"
#include "base/logging.h"

namespace base {

inline uint32 GetNumBits(uint32 value) {
  if (value == 0) {
    return 1;
  }
  uint32 num_bits = 0;
  while (value > 0) {
    ++num_bits;
    value >>= 1;
  }
  return num_bits;
}

inline void SetBit(uint32 index, uint8* data) {
  data[index >> 3] |= (1 << (index & 0x7));
}

inline void ClearBit(uint32 index, uint8* data) {
  data[index >> 3] &= ~(1 << (index & 0x7));
}

inline uint32 ReadBit(const uint32 index, const uint8* data) {
  return (data[index >> 3] >> (index & 0x7)) & 0x1;
}

inline uint8* ZipInt32WithGamma(
    const uint32* value_list,
    const uint32 value_list_len,
    uint8* target) {
  uint32 index = 0;
  for (int i = 0; i < value_list_len; ++i) {
    if (value_list[i] == 0) {
      // Encode the '0' value: 00
      ClearBit(index++, target);
      ClearBit(index++, target);
      continue;
    }
    if (value_list[i] == 1) {
      // Encode the '1' value: 01
      ClearBit(index++, target);
      SetBit(index++, target);
      continue;
    }
    uint32 num_bits = GetNumBits(value_list[i]);
    // Write the unary part.
    uint32 num_unary_bits = num_bits - 1;
    while (num_unary_bits--) {
      SetBit(index++, target);
    }
    ClearBit(index++, target);
    // Write the lower binary part.
    uint32 num_binary_bits = num_bits - 1;
    uint32 lower_binary_value = value_list[i] - (1 << (num_bits - 1));
    while (num_binary_bits) {
      if (lower_binary_value & 1) {
        SetBit(index + num_binary_bits - 1, target);
      } else {
        ClearBit(index + num_binary_bits - 1, target);
      }
      lower_binary_value >>= 1;
      --num_binary_bits;
    }
    index += (num_bits - 1);
  }
  return target + ((index - 1) >> 3) + 1;
}

inline const uint8* UnzipInt32WithGamma(
    const uint8* buffer,
    uint32* value_list,
    const uint32 value_list_len) {
  uint32 index = 0;
  for (int i = 0; i < value_list_len; ++i) {
    uint32 num_unary_bits = 0;
    while (ReadBit(index++, buffer)) {
      ++num_unary_bits;
    }
    if (num_unary_bits == 0) {
      // The original value is 0 or 1.
      value_list[i] = ReadBit(index++, buffer);
      continue;
    }
    uint32 lower_binary_value = 0;
    uint32 num_binary_bits = num_unary_bits;
    while (num_binary_bits--) {
      lower_binary_value =
        (lower_binary_value << 1) + ReadBit(index++, buffer);
    }
    value_list[i] = (1 << num_unary_bits) + lower_binary_value;
  }
  return buffer + ((index - 1) >> 3) + 1;
}
}  // namspace base
#endif  // BASE_GAMMA_COMPRESSION_H_
