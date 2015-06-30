#include "gtest/gtest.h"

#include <unistd.h>
#include "deps/base/logging.h"
#include "deps/base/flags.h"
#include "src/include_std.h"
#include "src/auth/auth.h"
#include "test/unittest/event_loop_setup.h"

DECLARE_string(auth_type);

namespace xcomet {

const char* const TEST_USER0 = "un_authed_user";
const char* const TEST_PWD0 = "un_authed_pwd";

const char* const TEST_USER1 = "54b6260cb8575c2784a05c95";
const char* const TEST_PWD1 = "dTxBq4Zkuj/amoApyrVmb6HZP7xPRo9Qi8FQKbAS4umQWMEJjy1Ii7JfXOW4WYoZLUatQ8lY3HeqYRyRcjlUZWJLuiAxEbJAp1zT4bTeM8aJSNm46NkapyMMn1LK9FlgDspChcGCaXaViyx8WLbm9a7gPtzngk4mfRRj/osjpgd++sRXggAybawKl4N+XC3iFSY3JQ/R4YpK02TIG5ydrZDdJOxLzNcH23jyBwLDdDwATZM/tJ2w+fxbHpCCnhxkro0AE2nzWQNGAStGv5jWcoabmGqldR2Ia+5cmPexnHI5OBfdSeohYUNskv+aCrIIVbyIXXNrzJGggSH5P3QVOw==";

const char* const TEST_USER2 = "un_matched_user";
const char* const TEST_PWD2 = "RAgP1i7r+jLQxFd+VmL/5NBNznPtmW/6YxoTQ4QOZvNptzTbxbAz1zE5jUemdXKRYD9SK7tFlmwV45EdVbeYlD8Lt85Xr74b220z6klJFm2RjjZTr121xJeoUrUDV9FRhcTfhabbMndrBCIEo7NYTJ7K/9MoiimxbMgIUTHOVEqygNDKH7QxAW5BNCSYj8Utcilgd2Qeulx/sGMSfLcrdqdlAg2Y19f3h/QPRVNu5pGitagstapOnn6S+NCPV17WTGIfU1UKhL+n+60zSs/SxVNHrj/wbw+kO2HnN5RKGuJTHkPCGmN9aqX1i1EVm0KobQw4zJSo4w8VbPOyQhcCPQ==";

class AuthUnittest : public testing::Test {
 public:
  std::thread::id MainThreadId() {
    return event_loop_setup_->MainThreadId();
  }
 protected:
  virtual void SetUp() {
    LOG(INFO) << "SetUp";
    event_loop_setup_ = new EventLoopSetup();
  }

  virtual void TearDown() {
    LOG(INFO) << "TearDown";
    delete event_loop_setup_;
  }

 private:
  EventLoopSetup* event_loop_setup_;
};

TEST_F(AuthUnittest, NoneAuth) {
  FLAGS_auth_type = "None";
  Auth auth;
  auth.Authenticate(TEST_USER0, TEST_PWD0, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER0, TEST_PWD0, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER1, TEST_PWD1, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER1, TEST_PWD1, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER2, TEST_PWD2, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER2, TEST_PWD2, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  ::sleep(1);
}

TEST_F(AuthUnittest, FastAuth) {
  FLAGS_auth_type = "Fast";
  Auth auth;
  auth.Authenticate(TEST_USER0, TEST_PWD0, [](Error err, bool ok) {
    CHECK(err != NO_ERROR);
    CHECK(!ok);
  });
  auth.Authenticate(TEST_USER0, TEST_PWD0, [](Error err, bool ok) {
    CHECK(err != NO_ERROR);
    CHECK(!ok);
  });
  auth.Authenticate(TEST_USER1, TEST_PWD1, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER1, TEST_PWD1, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER2, TEST_PWD2, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER2, TEST_PWD2, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  ::sleep(1);
}

TEST_F(AuthUnittest, FullAuth) {
  FLAGS_auth_type = "Full";
  Auth auth;
  auth.Authenticate(TEST_USER0, TEST_PWD0, [](Error err, bool ok) {
    CHECK(err != NO_ERROR);
    CHECK(!ok);
  });
  auth.Authenticate(TEST_USER0, TEST_PWD0, [](Error err, bool ok) {
    CHECK(err != NO_ERROR);
    CHECK(!ok);
  });
  auth.Authenticate(TEST_USER1, TEST_PWD1, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER1, TEST_PWD1, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER2, TEST_PWD2, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(!ok);
  });
  auth.Authenticate(TEST_USER2, TEST_PWD2, [](Error err, bool ok) {
    CHECK(err == NO_ERROR);
    CHECK(!ok);
  });
  ::sleep(1);
}

}  // namespace xcomet
