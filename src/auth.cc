#include "src/auth.h"

#include <string.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "deps/base/flags.h"
#include "deps/base/file.h"

using base::LRUCache;

DEFINE_string(auth_private_key_file, "./res/private/private.pem", "");
DEFINE_string(auth_type, "None", "None|Fast|Full");

const int DEFAULT_AUTH_LRU_CACHE_SIZE = 100000;

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

bool Auth::RSAPrivateDecode(const string& input, string& output) {
  if (input.empty()) {
    return false;
  }

  BIO* key_mem = BIO_new_mem_buf((void*)private_key_.c_str(),
                                 private_key_.size());
  RSA* rsa = RSA_new();
  if (PEM_read_bio_RSAPrivateKey(key_mem, &rsa, 0, 0) == NULL) {
    LOG(ERROR) << "read private key failed: " << input;
    RSA_free(rsa);
    BIO_free(key_mem);
    return false;
  }
  int rsa_len = RSA_size(rsa);
  output.resize(rsa_len + 1);
  int ret = RSA_private_decrypt(rsa_len,
                                (const unsigned char*)input.c_str(),
                                (unsigned char*)output.c_str(),
                                rsa,
                                RSA_PKCS1_PADDING);
  RSA_free(rsa);
  BIO_free(key_mem);
  return ret != -1;
}

Auth::Auth() : cache_(DEFAULT_AUTH_LRU_CACHE_SIZE) {
  base::File::ReadFileToStringOrDie(FLAGS_auth_private_key_file, &private_key_);
  if (FLAGS_auth_type == "None") {
    type_ = T_NONE;
  } else if (FLAGS_auth_type == "Fast") {
    type_ = T_FAST;
  } else if (FLAGS_auth_type == "Full") {
    type_ = T_FULL;
  } else {
    CHECK(false) << "unknow auth type";
  }
}

Auth::~Auth() {
}

static bool IsValidDeviceId(const Json::Value& id) {
  return id.isMember("AID") &&
         id.isMember("WMAC") &&
         id.isMember("BMAC") &&
         id.isMember("IMEI") &&
         id.isMember("PUID");
}

void Auth::Authenticate(const string& user,
                        const string& password,
                        AuthCallback cb) {
  if (type_ == T_NONE) {
    cb(NO_ERROR, true);
    return;
  }
  if (cache_.Get(password).get() != NULL) {
    cb(NO_ERROR, true);
    return;
  }
  VLOG(4) << "input password: " << password;
  string encryped_key;
  Base64Decode(password.c_str(), password.length(), encryped_key);
  VLOG(4) << "decode base64: " << encryped_key;
  string deviceid;
  if (!RSAPrivateDecode(encryped_key, deviceid)) {
    ErrorPtr error(new string("auth decode key failed"));
    cb(error, false);
    return;
  }
  VLOG(4) << "decoded device id: " << deviceid;
  Json::Value json;
  if (!Json::Reader().parse(deviceid, json) ||
      !IsValidDeviceId(json)) {
    ErrorPtr error(new string("auth invalid deviceid"));
    cb(error, false);
    return;
  }
  VLOG(4) << "unserialized device id: " << json;

  if (type_ == T_FULL) {
    // TODO(qingfeng) request mongodb for user authentiate
    ErrorPtr error(new string("request mongodb for auth not support yet"));
    cb(error, false);
    return;
  }
  cache_.Put(password, new bool(true));
  cb(NO_ERROR, true);
}

}  // namespace xcomet
