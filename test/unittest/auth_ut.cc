#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "src/include_std.h"
#include "src/auth.h"

namespace xcomet {

const char* const TEST_USER1 = "user1";
const char* const TEST_PWD1 = "dTxBq4Zkuj/amoApyrVmb6HZP7xPRo9Qi8FQKbAS4umQWMEJjy1Ii7JfXOW4WYoZLUatQ8lY3HeqYRyRcjlUZWJLuiAxEbJAp1zT4bTeM8aJSNm46NkapyMMn1LK9FlgDspChcGCaXaViyx8WLbm9a7gPtzngk4mfRRj/osjpgd++sRXggAybawKl4N+XC3iFSY3JQ/R4YpK02TIG5ydrZDdJOxLzNcH23jyBwLDdDwATZM/tJ2w+fxbHpCCnhxkro0AE2nzWQNGAStGv5jWcoabmGqldR2Ia+5cmPexnHI5OBfdSeohYUNskv+aCrIIVbyIXXNrzJGggSH5P3QVOw==";

const char* const TEST_USER2 = "user2";
const char* const TEST_PWD2 = "RAgP1i7r+jLQxFd+VmL/5NBNznPtmW/6YxoTQ4QOZvNptzTbxbAz1zE5jUemdXKRYD9SK7tFlmwV45EdVbeYlD8Lt85Xr74b220z6klJFm2RjjZTr121xJeoUrUDV9FRhcTfhabbMndrBCIEo7NYTJ7K/9MoiimxbMgIUTHOVEqygNDKH7QxAW5BNCSYj8Utcilgd2Qeulx/sGMSfLcrdqdlAg2Y19f3h/QPRVNu5pGitagstapOnn6S+NCPV17WTGIfU1UKhL+n+60zSs/SxVNHrj/wbw+kO2HnN5RKGuJTHkPCGmN9aqX1i1EVm0KobQw4zJSo4w8VbPOyQhcCPQ==";

TEST(AuthUnittest, Normal) {
  Auth auth;
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
}

}  // namespace xcomet
