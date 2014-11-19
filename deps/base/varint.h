//
// Most code ported from google/protobuf/io/coded_stream.cc

#ifndef BASE_VARINT_H_
#define BASE_VARINT_H_

#include "base/basictypes.h"
#include "base/logging.h"

namespace base {

static const int kMaxVarintBytes = 10;
static const int kMaxVarint32Bytes = 5;
static const uint64 kRiceModulus = 1 << 15;

inline uint8* WriteVarint32(uint32 value, uint8* target) {
  target[0] = static_cast<uint8>(value | 0x80);
  if (value >= (1 << 7)) {
    target[1] = static_cast<uint8>((value >>  7) | 0x80);
    if (value >= (1 << 14)) {
      target[2] = static_cast<uint8>((value >> 14) | 0x80);
      if (value >= (1 << 21)) {
        target[3] = static_cast<uint8>((value >> 21) | 0x80);
        if (value >= (1 << 28)) {
          target[4] = static_cast<uint8>(value >> 28);
          return target + 5;
        } else {
          target[3] &= 0x7F;
          return target + 4;
        }
      } else {
        target[2] &= 0x7F;
        return target + 3;
      }
    } else {
      target[1] &= 0x7F;
      return target + 2;
    }
  } else {
    target[0] &= 0x7F;
    return target + 1;
  }
}

inline uint32 GetVarint32Length(const uint64 value) {
  uint32 r = 1 << 7;
  uint32 c = 1;
  while (value >= r && c < 5) {
    r = r << 7;
    ++c;
  }
  return c;
}

inline const uint8* ReadVarint32(const uint8* buffer, uint32* value) {
  // Fast path:  We have enough bytes left in the buffer to guarantee that
  // this read won't cross the end, so we can skip the checks.
  const uint8* ptr = buffer;
  uint32 b;
  uint32 result;

  b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); result |=  b         << 28; if (!(b & 0x80)) goto done;  // NOLINT

  // If the input is larger than 32 bits, we still need to read it all
  // and discard the high-order bits.
  for (int i = 0; i < kMaxVarintBytes - kMaxVarint32Bytes; i++) {
    b = *(ptr++);
    if (!(b & 0x80)) goto done;
  }

  // We have overrun the maximum size of a varint (10 bytes).  Assume
  // the data is corrupt.
  return NULL;

 done:
  *value = result;
  return ptr;
}

inline uint8* WriteVarint64Fast(uint64 value, uint8* target) {
  CHECK(!(value & 0x1LLU<<63));
  uint64 mask = ~0x7f;
  if (!(value & mask)) {
    *target++ = value;
  } else {
    *target++ = (value >> 56) | 0x80;
    *target++ = (value >> 48) & 0xff;
    *target++ = (value >> 40) & 0xff;
    *target++ = (value >> 32) & 0xff;
    *target++ = (value >> 24) & 0xff;
    *target++ = (value >> 16) & 0xff;
    *target++ = (value >> 8) & 0xff;
    *target++ = value & 0xff;
  }

  return target;
}

inline uint8* WriteVarint64(uint64 value, uint8* target) {
  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  uint32 part0 = static_cast<uint32>(value);
  uint32 part1 = static_cast<uint32>(value >> 28);
  uint32 part2 = static_cast<uint32>(value >> 56);

  int size;

  // Here we can't really optimize for small numbers, since the value is
  // split into three parts.  Cheking for numbers < 128, for instance,
  // would require three comparisons, since you'd have to make sure part1
  // and part2 are zero.  However, if the caller is using 64-bit integers,
  // it is likely that they expect the numbers to often be very large, so
  // we probably don't want to optimize for small numbers anyway.  Thus,
  // we end up with a hardcoded binary search tree...
  if (part2 == 0) {
    if (part1 == 0) {
      if (part0 < (1 << 14)) {
        if (part0 < (1 << 7)) {
          size = 1;
          goto size1;
        } else {
          size = 2;
          goto size2;
        }
      } else {
        if (part0 < (1 << 21)) {
          size = 3;
          goto size3;
        } else {
          size = 4;
          goto size4;
        }
      }
    } else {
      if (part1 < (1 << 14)) {
        if (part1 < (1 << 7)) {
          size = 5;
          goto size5;
        } else {
          size = 6;
          goto size6;
        }
      } else {
        if (part1 < (1 << 21)) {
          size = 7;
          goto size7;
        } else {
          size = 8;
          goto size8;
        }
      }
    }
  } else {
    if (part2 < (1 << 7)) {
      size = 9;
      goto size9;
    } else {
      size = 10;
      goto size10;
    }
  }

  LOG(FATAL) << "Can't get here.";

  size10: target[9] = static_cast<uint8>((part2 >>  7) | 0x80);  // NOLINT
  size9 : target[8] = static_cast<uint8>((part2      ) | 0x80);  // NOLINT
  size8 : target[7] = static_cast<uint8>((part1 >> 21) | 0x80);  // NOLINT
  size7 : target[6] = static_cast<uint8>((part1 >> 14) | 0x80);  // NOLINT
  size6 : target[5] = static_cast<uint8>((part1 >>  7) | 0x80);  // NOLINT
  size5 : target[4] = static_cast<uint8>((part1      ) | 0x80);  // NOLINT
  size4 : target[3] = static_cast<uint8>((part0 >> 21) | 0x80);  // NOLINT
  size3 : target[2] = static_cast<uint8>((part0 >> 14) | 0x80);  // NOLINT
  size2 : target[1] = static_cast<uint8>((part0 >>  7) | 0x80);  // NOLINT
  size1 : target[0] = static_cast<uint8>((part0      ) | 0x80);  // NOLINT

  target[size-1] &= 0x7F;
  return target + size;
}

inline uint32 GetVarint64Length(const uint64 value) {
  uint64 r = 1 << 7;
  uint32 c = 1;
  while (value >= r && c < 10) {
    r = r << 7;
    ++c;
  }
  return c;
}

inline const uint8* ReadVarint64Fast(const uint8* ptr, uint64* value) {
  if (!(*ptr & 0x80)) {
    *value = *ptr++;
  } else {
    register uint64 b;
    b = (*ptr++) & 0x7f; *value = b << 56;  // NOLINT
    b = *ptr++; *value |= b << 48;  // NOLINT
    b = *ptr++; *value |= b << 40;  // NOLINT
    b = *ptr++; *value |= b << 32;  // NOLINT
    b = *ptr++; *value |= b << 24;  // NOLINT
    b = *ptr++; *value |= b << 16;  // NOLINT
    b = *ptr++; *value |= b << 8;   // NOLINT
    b = *ptr++; *value |= b;        // NOLINT
  }

  return ptr;
}

inline const uint8* ReadVarint64(const uint8* ptr, uint64* value) {
  uint64 b;

  uint64 part0 = 0;
  uint64 part1 = 0;

  b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) << 28; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) << 35; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) << 42; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part0 |= (b & 0x7F) << 49; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;  // NOLINT
  b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;  // NOLINT

  // We have overrun the maximum size of a varint (10 bytes).  The data
  // must be corrupt.
  return NULL;

 done:
  *value = (static_cast<uint64>(part0)      ) |  // NOLINT
           (static_cast<uint64>(part1) << 56);
  return ptr;
}

inline uint8* WriteRiceInt64(uint64 value, uint8* target) {
  // TODO(quj): not completed yet, only work for small number.
  CHECK_LT(value, 1 << 24);

  uint8 remainder1 = (value & 0xFF00) >> 8;
  uint8 remainder0 = value & 0xFF;

  if (value < kRiceModulus) {
    target[0] = remainder1 | 0x80;
    target[1] = remainder0;
    return target + 2;
  }

  uint64 q = value / kRiceModulus;
  uint64 h = 0;
  for (uint64 i = 1; i <= q; ++i) {
    h += 1 << i;
  }

  int modu_length = 0;
  if (q <= 6) {
    target[0] = static_cast<uint8>(h & 0xFF);
    modu_length = 1;
  } else if (q <= 14) {
    target[0] = static_cast<uint8>(h & 0xFF);
    target[1] = static_cast<uint8>((h >> 8) & 0xFF);
    modu_length = 2;
  } else if (q <= 22) {
    target[0] = static_cast<uint8>(h & 0xFF);
    target[1] = static_cast<uint8>((h >> 8) & 0xFF);
    target[2] = static_cast<uint8>((h >> 16) & 0xFF);
    modu_length = 3;
  } else if (q <= 30) {
    target[0] = static_cast<uint8>(h & 0xFF);
    target[1] = static_cast<uint8>((h >> 8) & 0xFF);
    target[2] = static_cast<uint8>((h >> 16) & 0xFF);
    target[3] = static_cast<uint8>((h >> 24) & 0xFF);
    modu_length = 4;
  } else if (q <= 38) {
    target[0] = static_cast<uint8>(h & 0xFF);
    target[1] = static_cast<uint8>((h >> 8) & 0xFF);
    target[2] = static_cast<uint8>((h >> 16) & 0xFF);
    target[3] = static_cast<uint8>((h >> 24) & 0xFF);
    target[4] = static_cast<uint8>((h >> 32) & 0xFF);
    modu_length = 5;
  } else if (q <= 46) {
    target[0] = static_cast<uint8>(h & 0xFF);
    target[1] = static_cast<uint8>((h >> 8) & 0xFF);
    target[2] = static_cast<uint8>((h >> 16) & 0xFF);
    target[3] = static_cast<uint8>((h >> 24) & 0xFF);
    target[4] = static_cast<uint8>((h >> 32) & 0xFF);
    target[5] = static_cast<uint8>((h >> 40) & 0xFF);
    modu_length = 6;
  } else if (q <= 54) {
    target[0] = static_cast<uint8>(h & 0xFF);
    target[1] = static_cast<uint8>((h >> 8) & 0xFF);
    target[2] = static_cast<uint8>((h >> 16) & 0xFF);
    target[3] = static_cast<uint8>((h >> 24) & 0xFF);
    target[4] = static_cast<uint8>((h >> 32) & 0xFF);
    target[5] = static_cast<uint8>((h >> 40) & 0xFF);
    target[6] = static_cast<uint8>((h >> 48) & 0xFF);
    modu_length = 7;
  }

  target[modu_length] = remainder1 & 0x7F;
  target[modu_length + 1] = remainder0;
  return target + modu_length + 2;
}

inline const uint8* ReadRiceInt64(const uint8* ptr, uint64* value) {
  uint64 b = *ptr;
  if (b & 0x80) {
    *value = ((b & 0x7F) << 8) | *(ptr + 1);
    return ptr + 2;
  } else {
    uint64 q = 0;
    while (true) {
      if ((b & 0x01) && (b & 0x80)) {
        q += 8;
      } else if (!(b & 0x01) && !(b & 0x80) && b) {
        b >>= 1;
        while (b) {
          q++;
          b >>= 1;
        }
        break;
      } else if (b & 0x01 && !(b & 0x80)) {
        while (b) {
          q++;
          b >>= 1;
        }
      } else if (!(b & 0x01) && (b & 0x80)) {
        q += 7;
      }

      b = *(ptr++);
    }
    ptr++;
    *value = kRiceModulus * q;
    *value += ((*ptr & 0x7F) << 8) | *(ptr + 1);
    return ptr + 1;
  }
}

inline uint8* WriteVarint64In2Bytes(uint64 value, uint8* target) {
  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  uint32 part0 = static_cast<uint32>(value);
  uint32 part1 = static_cast<uint32>(value >> 30);
  uint32 part2 = static_cast<uint32>(value >> 60);

  int size;
  uint16* result = reinterpret_cast<uint16*>(target);

  if (part2 == 0) {
    if (part1 == 0) {
      if (part0 < (1 << 15)) {
        size = 2;
        goto size2;
      } else {
        size = 4;
        goto size4;
      }
    } else {
      if (part1 < (1 << 15)) {
        size = 6;
        goto size6;
      } else {
        size = 8;
        goto size8;
      }
    }
  } else {
    size = 10;
    goto size10;
  }

  LOG(FATAL) << "Can't get here.";

  size10 : result[4] = static_cast<uint16>((part2      ) | 0x8000);  // NOLINT
  size8  : result[3] = static_cast<uint16>((part1 >> 15) | 0x8000);  // NOLINT
  size6  : result[2] = static_cast<uint16>((part1      ) | 0x8000);  // NOLINT
  size4  : result[1] = static_cast<uint16>((part0 >> 15) | 0x8000);  // NOLINT
  size2  : result[0] = static_cast<uint16>((part0      ) | 0x8000);  // NOLINT

  target[size-1] &= 0x7F;
  return target + size;
}


inline const uint8* ReadVarint64In2Bytes(const uint8* ptr, uint64* value) {
  uint64 b;

  uint64 part0 = 0;
  uint64 part1 = 0;

  const uint16* p = (const uint16*)(ptr);

  b = *(p++); part0  = (b & 0x7FFF)      ; if (!(b & 0x8000)) goto done;  // NOLINT
  b = *(p++); part0 |= (b & 0x7FFF) << 15; if (!(b & 0x8000)) goto done;  // NOLINT
  b = *(p++); part0 |= (b & 0x7FFF) << 30; if (!(b & 0x8000)) goto done;  // NOLINT
  b = *(p++); part0 |= (b & 0x7FFF) << 45; if (!(b & 0x8000)) goto done;  // NOLINT
  b = *(p++); part1  = (b & 0x7FFF)      ; if (!(b & 0x8000)) goto done;  // NOLINT

  // We have overrun the maximum size of a varint (10 bytes).  The data
  // must be corrupt.
  return NULL;

 done:
  *value = (static_cast<uint64>(part0)      ) |  // NOLINT
           (static_cast<uint64>(part1) << 60);
  return (const uint8*)(p);
}

}  // namespace base

#endif  // BASE_VARINT_H_
