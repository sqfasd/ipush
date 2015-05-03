
#include "base/hash.h"
#include "base/logging.h"
#include "base/string_util.h"

namespace {

// IMPORTANT: DON'T CHANGE THIS VALUE.
const uint32 kFingerPrintSeed = 19861225;
}

namespace base {

uint64 Fingerprint(const StringPiece& str) {
  return MurmurHash64A(str.data(),
                       str.size(),
                       kFingerPrintSeed);
}

uint64 Fingerprint(const void* key, int len) {
  return MurmurHash64A(key, len, kFingerPrintSeed);
}

uint32 Fingerprint32(const StringPiece& str) {
  return MurmurHash32A(str.data(),
                       str.size(),
                       kFingerPrintSeed);
}

uint32 Fingerprint32(const void* key, int len) {
  return MurmurHash32A(key, len, kFingerPrintSeed);
}

void FingerprintToString(uint64 fp, std::string* str) {
  SStringPrintf(str, "%.16lx", fp);
}

std::string FingerprintToString(uint64 fp) {
  return StringPrintf("%.16lx", fp);
}

uint64 StringToFingerprint(const std::string& str) {
  return strtoul(str.c_str(), NULL, 16);
}

// 64-bit hash for 64-bit platforms
uint64 MurmurHash64A(const void* key, int len, uint32 seed) {
  const uint64 m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64 h = seed ^ (len * m);

  const uint64* data = (const uint64 *)key;
  const uint64* end = data + (len/8);

  while (data != end) {
    uint64 k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const uint8* data2 = (const uint8*)data;

  switch (len & 7) {
    case 7: h ^= static_cast<uint64>(data2[6]) << 48;
    case 6: h ^= static_cast<uint64>(data2[5]) << 40;
    case 5: h ^= static_cast<uint64>(data2[4]) << 32;
    case 4: h ^= static_cast<uint64>(data2[3]) << 24;
    case 3: h ^= static_cast<uint64>(data2[2]) << 16;
    case 2: h ^= static_cast<uint64>(data2[1]) << 8;
    case 1: h ^= static_cast<uint64>(data2[0]);
    h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

// 32-bit hash
uint32 MurmurHash32A(const void* key, int len, uint32 seed) {
  const uint32 m = 0x5bd1e995;
  const int r = 24;

  uint32 h = seed ^ (len * m);

  const uint32* data = (const uint32 *)key;

  while (len >= 4) {
    uint32 k = *(uint32 *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 1;
    len -= 4;
  }

  // Handle the last few bytes of the input array
 const uint8* data2 = (const uint8*)data;

  switch (len) {
    case 3: h ^= static_cast<uint32>(data2[2]) << 16;
    case 2: h ^= static_cast<uint32>(data2[1]) << 8;
    case 1: h ^= static_cast<uint32>(data2[0]);
            h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

uint32 MurmurHash3_32(const void* key, int len, uint32 seed) {
  const uint8* data = (const uint8*)key;
  const int nblocks = len / 4;

  uint32 h1 = seed;

  uint32 c1 = 0xcc9e2d51;
  uint32 c2 = 0x1b873593;

  //----------
  // body

  const uint32* blocks = (const uint32*)(data + nblocks*4);

  for (int i = -nblocks; i; i++) {
    uint32 k1 = blocks[i];

    k1 *= c1;
    k1 = (k1 << 15) | (k1 >> (32 - 15));
    k1 *= c2;

    h1 ^= k1;
    h1 = (h1 << 13) | (h1 >> (32 - 13));
    h1 = h1*5 + 0xe6546b64;
  }

  //----------
  // tail

  const uint8* tail = (const uint8*)(data + nblocks*4);

  uint32 k1 = 0;

  switch (len & 3) {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];

      k1 *= c1;
      k1 = (k1 << 15) | (k1 >> (32 - 15));
      k1 *= c2;
      h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len;

  h1 ^= h1 >> 16;
  h1 *= 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 *= 0xc2b2ae35;
  h1 ^= h1 >> 16;

  return h1;
}

}

namespace {

const int kFNV_128_DIGEST_SIZE = 16;

// 128bit prime = 2^88 + 2^8 + 0x3b
const uint32 kFNV_128_PRIME_LOW = 0x13b;
const uint32 kFNV_128_PRIME_SHIFT = 24;

enum CHashParserSate {
  kInTag = 0,
  kInScript,
  kInTitle,
  kInText,
  kInStyle,
};

void FNVUpdate(const char c, uint64* val) {
  val[0] ^= static_cast<uint64>(c);

  // Multiplication.
  uint64 tmp[4];

  tmp[0] = val[0] * kFNV_128_PRIME_LOW;
  tmp[1] = val[1] * kFNV_128_PRIME_LOW;
  tmp[2] = val[2] * kFNV_128_PRIME_LOW;
  tmp[3] = val[3] * kFNV_128_PRIME_LOW;

  tmp[2] += val[0] << kFNV_128_PRIME_SHIFT;
  tmp[3] += val[1] << kFNV_128_PRIME_SHIFT;

  // Propagate carries.
  tmp[1] += (tmp[0] >> 32);
  val[0] = tmp[0] & 0x00000000ffffffffULL;

  tmp[2] += (tmp[1] >> 32);
  val[1] = tmp[1] & 0x00000000ffffffffULL;

  val[3] = tmp[3] + (tmp[2] >> 32);
  val[2] = tmp[2] & 0x00000000ffffffffULL;
}

bool IsScriptTag(const char* data) {
  if ((data[0] == 's' || data[0] == 'S') &&
      (data[1] == 'c' || data[1] == 'C') &&
      (data[2] == 'r' || data[2] == 'R') &&
      (data[3] == 'i' || data[3] == 'I') &&
      (data[4] == 'p' || data[4] == 'P') &&
      (data[5] == 't' || data[5] == 'T')) {
    return true;
  }
  return false;
}

bool IsStyleTag(const char* data) {
  if ((data[0] == 's' || data[0] == 'S') &&
      (data[1] == 't' || data[1] == 'T') &&
      (data[2] == 'y' || data[2] == 'Y') &&
      (data[3] == 'l' || data[3] == 'L') &&
      (data[4] == 'e' || data[4] == 'E')) {
    return true;
  }
  return false;
}

bool IsTitleTag(const char* data) {
  if ((data[0] == 't' || data[0] == 'T') &&
      (data[1] == 'i' || data[1] == 'I') &&
      (data[2] == 't' || data[2] == 'T') &&
      (data[3] == 'l' || data[3] == 'L') &&
      (data[4] == 'e' || data[4] == 'E')) {
    return true;
  }
  return false;
}

bool IsMetaTag(const char* data) {
  if ((data[0] == 'm' || data[0] == 'M') &&
      (data[1] == 'e' || data[1] == 'E') &&
      (data[2] == 't' || data[2] == 'T') &&
      (data[3] == 'a' || data[3] == 'A')) {
    return true;
  }
  return false;
}

}

namespace base {

void ContentHash(const char* data, int len, void* digest) {
  // Initial hash value: 6C62272E 07BB0142 62B82175 6295C58D
  struct {
    uint64 word[2];
  } HashVal;

  HashVal.word[0] = 0x62B821756295C58DULL;
  HashVal.word[1] = 0x6C62272E07BB0142ULL;

  // If DataLen is zero, then hashing is not needed.
  if (len == 0) {
    memcpy(digest, HashVal.word, kFNV_128_DIGEST_SIZE);
    return;
  }

  uint64 val[4];

  val[0] = (HashVal.word[0] & 0x00000000ffffffffULL);
  val[1] = (HashVal.word[0] >> 32);
  val[2] = (HashVal.word[1] & 0x00000000ffffffffULL);
  val[3] = (HashVal.word[1] >> 32);

  int i = 0;
  CHashParserSate state = kInText;
  while (i < len) {
    switch (data[i]) {
      case '<': {
        // End tag </XX>
        if (i < len - 1 &&
            data[i+1] == '/') {
          if (state != kInScript &&
              state != kInStyle &&
              state != kInTitle) {
            while (i < len && data[i] != '>') {
              ++i;
            }
            state = kInText;
          } else {
            ++i;
            // </script ...
            if (i < len - 7 && IsScriptTag(data+i+1)) {
              i += 6;
              // handle </script     >
              state = kInTag;

            // </style ...
            } else if (state == kInStyle &&
                       i < len - 6 && IsStyleTag(data+i+1)) {
              i += 5;
              // handle </style     >
              state = kInTag;
            // </title ...
            } else if (state == kInTitle &&
                       i < len - 6 && IsTitleTag(data+i+1)) {
              i += 5;
              state = kInTag;
            }
          }

        // <script ...
        } else if (i < len - 7 && IsScriptTag(data+i+1)) {
          state = kInScript;
          i += 6;
          while (i < len && data[i] != '>') {
            ++i;
          }
        // <style ...
        } else if (i < len - 6 && IsStyleTag(data+i+1)) {
          state = kInStyle;
          i += 5;
          while (i < len && data[i] != '>') {
            ++i;
          }
        // <title ...
        } else if (state != kInScript &&
                   i < len - 6 && IsTitleTag(data+i+1)) {
          state = kInTitle;
          i += 5;
          while (i < len && data[i] != '>') {
            ++i;
          }
        // <meta ...
        } else if (state != kInScript &&
                   i < len - 5 && IsMetaTag(data+i+1)) {
          state = kInText;
          i += 5;
          while (i < len && data[i] != '>') {
            if (data[i] != ' ' &&
                data[i] != '\n' &&
                data[i] != '\r' &&
                data[i] != '\t' &&
                data[i] != '\"' &&
                data[i] != '/') {
              FNVUpdate(data[i], val);
            }
            ++i;
          }
        // <XXX
        } else if (state != kInScript &&
                   state != kInStyle &&
                   state != kInTitle) {
          state = kInTag;
        }

        ++i;
        break;
      }

      case '>': {
        if (state == kInTag) {
          state = kInText;
        }
        ++i;
        break;
      }

      case 's': case 'S': {
        // <XX src="...
        // TODO(quj): Can't handle <XX src = " ...
        if (state == kInTag &&
            i < len - 5 &&
            (data[i-1] == ' ' || data[i-1] == '\n' || data[i-1] == '\r') &&
            (data[i+1] == 'r' || data[i+1] == 'R') &&
            (data[i+2] == 'c' || data[i+2] == 'C') &&
            data[i+3] == '=' &&
            (data[i+4] == '\"' || data[i+4] == '\'')) {
          i += 5;
          while (i < len && data[i] != '\'' && data[i] != '\"') {
            FNVUpdate(data[i], val);
            ++i;
          }
        } else if (state == kInText ||
                   state == kInTitle ||
                   state == kInScript) {
          FNVUpdate(data[i], val);
        }
        ++i;
        break;
      }

      case 'h': case 'H': {
        // <XX href="...
        // TODO(quj): Can't handle <XX href = " ...
        if (state == kInTag &&
            i < len - 6 &&
            (data[i-1] == ' ' || data[i-1] == '\n' || data[i-1] == '\r') &&
            (data[i+1] == 'r' || data[i+1] == 'R') &&
            (data[i+2] == 'e' || data[i+2] == 'E') &&
            (data[i+3] == 'f' || data[i+3] == 'F') &&
            data[i+4] == '=' &&
            (data[i+5] == '\"' || data[i+5] == '\'')) {
          i += 6;
          while (i < len && data[i] != '\'' && data[i] != '\"') {
            FNVUpdate(data[i], val);
            ++i;
          }
        } else if (state == kInText ||
                   state == kInTitle ||
                   state == kInScript) {
          FNVUpdate(data[i], val);
        }
        ++i;
        break;
      }

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        if (state == kInTitle || state == kInScript) {
          FNVUpdate(data[i], val);
        }
        ++i;
        break;
      }
      case ' ': case '\n': case '\r': case '\t': {
        ++i;
        break;
      }
      default: {
        if (state == kInTitle || state == kInScript || state == kInText) {
          FNVUpdate(data[i], val);
        }
        ++i;
      }
    }
  }

  HashVal.word[1] = ((val[3]<<32) | val[2]);
  HashVal.word[0] = ((val[1]<<32) | val[0]);

  memcpy(digest, HashVal.word, kFNV_128_DIGEST_SIZE);
}

void ContentHashToString(const uint64* digest, std::string* str) {
  FingerprintToString(digest[0], str);
  std::string last;
  FingerprintToString(digest[1], &last);
  str->append(last);
}

void FNV128(const char* data, int len, void* digest) {
  if (data == NULL || digest == NULL || len < 0) {
    LOG(FATAL) << "Invalid parameter! fnv128 returned.";
    return;
  }

  // Initial hash value: 6C62272E 07BB0142 62B82175 6295C58D
  struct {
    uint64 word[2];
  } HashVal;

  HashVal.word[0] = 0x62B821756295C58DULL;
  HashVal.word[1] = 0x6C62272E07BB0142ULL;

  // If DataLen is zero, then hashing is not needed.
  if (len == 0) {
    memcpy(digest, HashVal.word, kFNV_128_DIGEST_SIZE);
    return;
  }

  uint64 val[4];

  val[0] = (HashVal.word[0] & 0x00000000ffffffffULL);
  val[1] = (HashVal.word[0] >> 32);
  val[2] = (HashVal.word[1] & 0x00000000ffffffffULL);
  val[3] = (HashVal.word[1] >> 32);

  int pos = 0;
  while (pos < len) {
    val[0] ^= static_cast<uint64>(*data++);
    ++pos;

    // Multiplication
    uint64 tmp[4];

    tmp[0] = val[0] * kFNV_128_PRIME_LOW;
    tmp[1] = val[1] * kFNV_128_PRIME_LOW;
    tmp[2] = val[2] * kFNV_128_PRIME_LOW;
    tmp[3] = val[3] * kFNV_128_PRIME_LOW;

    tmp[2] += val[0] << kFNV_128_PRIME_SHIFT;
    tmp[3] += val[1] << kFNV_128_PRIME_SHIFT;

    // Propagate carries
    tmp[1] += (tmp[0] >> 32);
    val[0] = tmp[0] & 0x00000000ffffffffULL;

    tmp[2] += (tmp[1] >> 32);
    val[1] = tmp[1] & 0x00000000ffffffffULL;

    val[3] = tmp[3] + (tmp[2] >> 32);
    val[2] = tmp[2] & 0x00000000ffffffffULL;
  }

  HashVal.word[1] = ((val[3]<<32) | val[2]);
  HashVal.word[0] = ((val[1]<<32) | val[0]);

  memcpy(digest, HashVal.word, kFNV_128_DIGEST_SIZE);
}

}  // namespace base
