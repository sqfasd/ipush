#include "src/auth/auth_db.h"

#include <string.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <sstream>

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "deps/base/flags.h"
#include "deps/base/file.h"
#include "deps/base/md5.h"
#include "deps/base/string_util.h"
#include "src/auth/mongo_client.h"
#include "src/loop_executor.h"
#include "src/utils.h"
#include "src/crypto/base64.h"

using base::LRUCache;

DEFINE_string(auth_private_key_file, "./res/private/private.pem", "");
DEFINE_string(auth_type, "None", "None|Fast|Full");
DEFINE_string(device_id_fields,
              "AID,WMAC,BMAC,IMEI,PUID",
              "");

const int DEFAULT_AUTH_LRU_CACHE_SIZE = 100000;

namespace xcomet {

bool AuthDB::RSAPrivateDecode(const string& input, string& output) {
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

AuthDB::AuthDB()
    : cache_(DEFAULT_AUTH_LRU_CACHE_SIZE) {
  LOG(INFO) << "AuthDB()";
  if (FLAGS_auth_type == "None") {
    type_ = T_NONE;
  } else if (FLAGS_auth_type == "Fast") {
    type_ = T_FAST;
  } else if (FLAGS_auth_type == "Full") {
    type_ = T_FULL;
    mongo_.reset(new MongoClient());
  } else {
    CHECK(false) << "unknow auth type";
  }
  if (type_ > T_NONE) {
    base::File::ReadFileToStringOrDie(FLAGS_auth_private_key_file,
                                      &private_key_);
    SplitString(FLAGS_device_id_fields, ',', &device_id_fields_);
  }
}

AuthDB::~AuthDB() {
  LOG(INFO) << "~AuthDB()";
}

static bool IsValidDeviceId(const Json::Value& id,
                            const vector<string>& required_fields) {
  for (int i = 0; i < required_fields.size(); ++i) {
    if (!id.isMember(required_fields[i])) {
      return false;
    }
  }
  return true;
}

static string JoinDeviceId(const Json::Value& id,
                           const vector<string>& required_fields) {
  std::ostringstream stream;
  for (int i = 0; i < required_fields.size(); ++i) {
    stream << id[required_fields[i]].asCString();
  }
  return stream.str();
}

void AuthDB::Authenticate(const string& user,
                        const string& password,
                        AuthCallback cb) {
  if (type_ == T_NONE) {
    cb(NO_ERROR, true);
    return;
  }
  base::shared_ptr<bool> cache_value;
  if (cache_.Get(user, &cache_value)) {
    cb(NO_ERROR, true);
    return;
  }
  VLOG(4) << "input password: " << password;
  string encryped_key = Base64Decode(password);
  VLOG(4) << "decode base64: " << encryped_key;
  string deviceid;
  if (!RSAPrivateDecode(encryped_key, deviceid)) {
    cb("auth decode key failed", false);
    return;
  }
  VLOG(4) << "decoded device id: " << deviceid;
  Json::Value json;
  if (!Json::Reader().parse(deviceid, json) ||
      !IsValidDeviceId(json, device_id_fields_)) {
    cb("auth invalid deviceid", false);
    return;
  }
  VLOG(4) << "unserialized device id: " << json;

  if (type_ == T_FULL) {
    string joined_devid = JoinDeviceId(json, device_id_fields_);
    VLOG(4) << "joined deviceid: " << joined_devid;
    StringPtr md5_devid(new string());
    *md5_devid = base::MD5String(joined_devid);
    VLOG(4) << "md5 deviceid: " << *md5_devid;
    mongo_->GetUserNameByDevId(md5_devid,
                               [user, cb, this](Error err, StringPtr name) {
      if (err != NO_ERROR) {
        LoopExecutor::RunInMainLoop(bind(cb, err, false));
      } else if (name.get() == NULL || *name != user) {
        LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR, false));
      } else {
        cache_.Put(user, new bool(true));
        LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR, true));
      }
    });
  } else {
    cache_.Put(user, new bool(true));
    cb(NO_ERROR, true);
  }
}

}  // namespace xcomet
