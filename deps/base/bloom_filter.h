
#ifndef BASE_BLOOM_FILTER_H_
#define BASE_BLOOM_FILTER_H_

#include <math.h>
#include <sys/types.h>  // for size_t
#include <climits>  // for CHAR_BIT

#include "base/basictypes.h"

namespace base {
namespace bloom {

template <typename T> struct FNV {};

// FNV params for the 32-bit hash function
template<> struct FNV<uint32> {
  static const uint32 PRIME = 16777619U;
  static const uint32 INIT = 2166136261U;
  static const uint32 MAX_BIT = 0xffffffffU;
};

// FNV params for the 64-bit hash function
template<> struct FNV<uint64> {
  static const uint64 PRIME = 1099511628211ULL;
  static const uint64 INIT = 14695981039346656037ULL;
  static const uint64 MAX_BIT = 0xffffffffffffffffULL;
};

// FNV hash function
// see http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash
template<typename T> T FNVHash(const uint8 *buf,
                               size_t len, T hval = FNV<T>::INIT) {
  const uint8 *bp = buf;  // start of buffer
  const uint8 *be = bp + len;  // beyond end of buffer
  while (bp < be) {  // FNV-1 hash each octet in the buffer
    hval *= FNV<T>::PRIME;  // multiply by the FNV magic prime mod 2^32
    hval ^= static_cast<T>(*bp++);  // xor the bottom with the current octet
  }
  return hval;  // return our new hash value
}

// Bit sequence structure
struct BitStorage {
  // Pointer to the bit sequence
  unsigned char* bit_set;
  // Size of bit sequence
  size_t bit_set_size;
  BitStorage(): bit_set(0), bit_set_size(0) {}
  BitStorage(uint8* bit_set, size_t bit_set_size) :
      bit_set(bit_set), bit_set_size(bit_set_size) {}
};

// Bloom filter realization @see http://en.wikipedia.org/wiki/Bloom_filter
//    @param T type of handling object (must be C++ POD type)
//    @param HASHT type of the hash function return-value
template<typename T, typename HASHT> class Bloom {
  /** Max number of the keeping elements */
  uint64 max_elements_number;
  /** Probability of positive false in percent (can have got value between 0.0
   * and 100.0) */
  double positive_false;
  /** Min hash functions number for the concrete max_elements_number and positieve_false.
      This value should be calculated by program but it can be defined by user */
  uint64 min_hash_f_number;
  /** Number of bits for the max_elements_number (will been calculated) */
  uint64  bits_number;
  /** Min bytes number for the bits_number storing */
  uint64 bytes_number;
  /** Initialization vector for the hash function (can be 0) */
  HASHT iv;
  /** Pointer to the hash function */
  HASHT (*p_hash_f)(const uint8 *buf, size_t len, HASHT iv);
  /** Bit sequence structure */
  BitStorage bset;

 public:
  /** Constructor
      @param max_elements_number max number of the keeping elements
      @param positive_false positive false in percent
      @param iv initialization vector for the hash function (can be 0)
      @param p_hash_f pointer to the hash function*/
  Bloom(uint64 max_elements_number, double positive_false, HASHT iv,
        HASHT (*p_hash_f)(const uint8 *buf, size_t len, HASHT iv)):
      max_elements_number(max_elements_number),
      positive_false(positive_false),
      iv(iv),
      p_hash_f(p_hash_f),
      bset() {
    bits_number = calculateBitsNumber(max_elements_number, positive_false);
    bytes_number = calcBytesNumber(bits_number);
    min_hash_f_number = calcMinHashFNumber(max_elements_number, bits_number);
  }

  /** Constructor
      @param max_elements_number max number of the storing elements
      @param positive_false positive false in percent
      @param min_hash_f_number min hash functions number
      @param iv initialization vector for the hash function (can be 0)
      @param p_hash_f pointer to the hash function*/
  Bloom(uint64 max_elements_number, double positive_false,
        uint64 min_hash_f_number, HASHT iv,
        HASHT (*p_hash_f)(const uint8 *buf, size_t len, HASHT iv)):
      max_elements_number(max_elements_number),
      positive_false(positive_false),
      min_hash_f_number(min_hash_f_number),
      iv(iv),
      p_hash_f(p_hash_f),
      bset() {
    bits_number = calculateBitsNumber(max_elements_number, positive_false);
    bytes_number = calcBytesNumber(bits_number);
  }

  /** Constructor
      @param max_elements_number max number of the storing elements
      @param bits_number number of bits in the sequence
      @param min_hash_f_number min hash functions number
      @param iv initialization vector for the hash function (can be 0)
      @param p_hash_f pointer to the hash function*/
  Bloom(uint64 max_elements_number, uint64 min_hash_f_number,
        HASHT (*p_hash_f)(const uint8 *buf, size_t len, HASHT iv),
        uint64 bits_number, HASHT iv):
      max_elements_number(max_elements_number),
      min_hash_f_number(min_hash_f_number),
      bits_number(bits_number),
      iv(iv),
      p_hash_f(p_hash_f),
      bset() {
    positive_false = calcPositiveFalse(max_elements_number, bits_number);
    bytes_number = calcBytesNumber(bits_number);
  }

  /** Copy-Constructor */
  Bloom(const Bloom& b) {
    max_elements_number = b.max_elements_number;
    positive_false = b.positive_false;
    min_hash_f_number = b.min_hash_f_number;
    iv = b.iv;
    p_hash_f = b.p_hash_f;
    bits_number = b.bits_number;
    bytes_number = b.bytes_number;
    bset = b.bset;
  }

  /** Destructor */
  virtual ~Bloom() {}

  /** Define bit sequence for the bloom calculation
      @param bit_set pointer to the sequence
      @param bit_set_size size of sequence (in bytes) */
  void setBitStorage(uint8* bit_set, size_t bit_set_size) {
    bset.bit_set = bit_set, bset.bit_set_size = bit_set_size;
  }

  /** Get current bit sequence */
  const BitStorage& getBitStorage() const {return bset;}

  /** Fill bits in the char sequence for the some element
      @param element pointer to the element
      @return true if the element is new */
  bool fillBitSet(T* element) {
    bool collision = true;
    if (element && bset.bit_set && bset.bit_set_size) {
      HASHT hash = 0, bitmask = 0, bitslot = 0;
      for (HASHT i = iv; i < (min_hash_f_number + iv); ++i) {
        hash = (*p_hash_f)(
            reinterpret_cast<const uint8*>(element), sizeof(T), i);
        // ? is hash %= bits_number ? ;
        hash %= bset.bit_set_size;
        bitmask = 1 << (hash % CHAR_BIT);
        bitslot = hash / CHAR_BIT;

        if (!(bset.bit_set[bitslot] & bitmask)) {
          collision = false;
          bset.bit_set[bitslot] |= bitmask;
        }
      }
    }
    return !collision;
  }

  /** Find bits in the char sequence for the some element
      @param element pointer to the element
      @return true if all bits are present */
  bool isElementPresent(T* element) {
    bool collision = true;
    if (element && bset.bit_set && bset.bit_set_size) {
      HASHT hash = 0, bitmask = 0, bitslot = 0;
      for (HASHT i = iv; i < (min_hash_f_number + iv); ++i) {
        hash = (*p_hash_f)(
            reinterpret_cast<const uint8*>(element), sizeof(T), i);
        hash %= bset.bit_set_size;
        bitmask = 1 << (hash % CHAR_BIT);
        bitslot = hash / CHAR_BIT;
        if (!(bset.bit_set[bitslot] & bitmask)) {
          collision = false;
          break;
        }
      }
    } else {
      collision = false;
    }
    return collision;
  }

  /** Calculate the number of hash functions
      @param max_elements_number max number of the storing elements
      @param bits_number number of bits in the sequence
      @return number of functions or 0*/
  static uint64 calcMinHashFNumber(uint64 max_elements_number,
                                   uint64 bits_number) {
    if (bits_number > 0) {
      return static_cast<uint64>(
          static_cast<double>(bits_number) /
          (static_cast<double>(max_elements_number)) * log(2.));
    }
    return 0;
  }

  /** Calculate the number of bits
      @param max_elements_number max number of the storing elements
      @param positive_false probability of positive false in percent
      @return number of bits or 0 */
  static uint64 calculateBitsNumber(uint64 max_elements_number,
                                    double positive_false) {
    if (0. < positive_false && positive_false < 100.) {
      return static_cast<uint64>(
          - static_cast<double>(max_elements_number) *
          log(positive_false / 100.) / pow(log(2.), 2.0));
    }
    return 0;
  }

  /** Calculate bytes number
      @param bits_number number of bits in the sequence
      @return number of bytes */
  static uint64 calcBytesNumber(uint64  bits_number) {
    uint64 bytes_number = bits_number / CHAR_BIT;
    if ((bits_number % CHAR_BIT) > 0) {
      ++bytes_number;
    }
    return bytes_number;
  }

  /** Calculate positive_false
      @param max_elements_number max number of the storing elements
      @param bits_number number of bits in the sequence
      @return positive_false */
  static double calcPositiveFalse(uint64 max_elements_number,
                             uint64 bits_number) {
    return - 100. * (exp(static_cast<double>(bits_number) *
           pow(log(2.), 2.0) /
           static_cast<double>(max_elements_number)));
  }
  /** Fill bits in the char sequence for the some element
      @param hash_set pointer to the hash_set
      @param nhash hash_set's len
      @return true if the element is new */
  bool FillBitSetWithHash(HASHT* hash_set, uint32 nhash) {
    bool collision = true;
    HASHT hash_bits_number = bits_number;
    if (hash_set && bset.bit_set && bset.bit_set_size) {
      HASHT hash = 0, bitmask = 0, bitslot = 0;
      for (int32 i = 0; i < nhash; ++i) {
        hash = *hash_set;
        hash = abs(hash % hash_bits_number);
        bitmask = 1 << (hash % CHAR_BIT);
        bitslot = hash / CHAR_BIT;
        if (!(bset.bit_set[bitslot] & bitmask)) {
          collision = false;
          bset.bit_set[bitslot] |= bitmask;
        }
        ++hash_set;
      }
    }
    return !collision;
  }
  /** Find bits in the char sequence for the some element
      @param hash_set pointer to the hash_set
      @param nhash hash_set's len
      @return true if all bits are present */
  bool isElementPresentWithHash(HASHT* hash_set, uint32 nhash) {
    bool collision = true;
    HASHT hash_bits_number = bits_number;
    if (hash_set && bset.bit_set && bset.bit_set_size) {
      HASHT hash = 0, bitmask = 0, bitslot = 0;
      for (int32 i = 0; i < nhash; ++i) {
        hash = *hash_set;
        hash = abs(hash % hash_bits_number);
        bitmask = 1 << (hash % CHAR_BIT);
        bitslot = hash / CHAR_BIT;
        if (!(bset.bit_set[bitslot] & bitmask)) {
          collision = false;
          break;
        }
        ++hash_set;
      }
    } else {
      collision = false;
    }
    return collision;
  }

  /** Return max_elements_number */
  uint64 getMaxElementsNumber() const {return max_elements_number;}
  /** Return positive_false */
  double getPositiveFalse() const {return positive_false;}
  /** Return min_hash_f_number */
  uint64 getMinHashFNumber() const {return min_hash_f_number;}
  /** Return bits_number */
  uint64 getBitsNumber() const {return bits_number;}
  /** Return bytes_number */
  uint64 getBytesNumber() const {return bytes_number;}
};
}
}

#endif  // BASE_BLOOM_FILTER_H_
