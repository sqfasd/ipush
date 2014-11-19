
#ifndef BASE_HASH_H_
#define BASE_HASH_H_

#include <string>
#include "base/basictypes.h"
#include "base/string_piece.h"

namespace base {

uint64 Fingerprint(const StringPiece& str);

uint64 Fingerprint(const void* key, int len);

uint32 Fingerprint32(const StringPiece& str);

uint32 Fingerprint32(const void* key, int len);

void FingerprintToString(uint64 fp, std::string* str);

std::string FingerprintToString(uint64 fp);

uint64 StringToFingerprint(const std::string& str);

// MurmurHash2, 64-bit versions, by Austin Appleby.
// http://sites.google.com/site/murmurhash/
uint64 MurmurHash64A(const void* key, int len, uint32 seed);

uint32 MurmurHash32A(const void* key, int len, uint32 seed);

uint32 MurmurHash3_32(const void* key, int len, uint32 seed);

static const uint64 kEmptyContentHashLow = 7113472399480571277UL;
static const uint64 kEmptyContentHashHigh = 7809847782465536322UL;

// Strip html tags, numbers and generate 128 bits content hash
// using FNV.
// Sample useage:
//   uint64 digest[2];
//   ContentHash(data, len, &digest)
//
// Expections:
// - keep all content in <title>, <script> tag.
// - remove all content in <style> tag.
// - keep content in src="" and href="".
void ContentHash(const char* data, int len, void* digest);

void ContentHashToString(const uint64* digest, std::string* str);

// 128 bits FNV checksum.
// Sample useage:
//   uint64 digest[2];
//   fnv128(data, len, &digest)
//
void FNV128(const char* data, int len, void* digest);

}  // namespace base

#endif  // BASE_HASH_H_
