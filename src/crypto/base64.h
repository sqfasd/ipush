#ifndef SRC_CRYPTO_BASE64_H_
#define SRC_CRYPTO_BASE64_H_

#include <string>

std::string Base64Encode(unsigned char const* , unsigned int len);
std::string Base64Decode(std::string const& s);

#endif  // SRC_CRYPTO_BASE64_H_
