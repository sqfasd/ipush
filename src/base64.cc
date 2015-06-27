#include <string.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include "src/base64.h"

namespace xcomet {

void Base64Encode(const char* input, int length, string& output) {
    BIO* bmem = NULL;
    BIO* b64 = NULL;
    BUF_MEM* bptr = NULL;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    output.resize(bptr->length + 1);
    ::memcpy((void*)output.c_str(), bptr->data, bptr->length);
    output[bptr->length] = 0;

    BIO_free_all(b64);
}

void Base64Decode(const char* input, int length, string& output) {
    BIO* b64 = NULL;
    BIO* bmem = NULL;
    output.resize(length);

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf((void*)input, length);
    bmem = BIO_push(b64, bmem);
    BIO_read(bmem, (void*)output.c_str(), length);

    BIO_free_all(bmem);
}

}  // namespace xcomet
