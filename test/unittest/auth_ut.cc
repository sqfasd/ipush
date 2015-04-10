#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "src/include_std.h"
#include "src/auth.h"

namespace xcomet {

const char* const TEST_USER = "user";
const char* const TEST_PWD = "dTxBq4Zkuj/amoApyrVmb6HZP7xPRo9Qi8FQKbAS4umQWMEJjy1Ii7JfXOW4WYoZLUatQ8lY3HeqYRyRcjlUZWJLuiAxEbJAp1zT4bTeM8aJSNm46NkapyMMn1LK9FlgDspChcGCaXaViyx8WLbm9a7gPtzngk4mfRRj/osjpgd++sRXggAybawKl4N+XC3iFSY3JQ/R4YpK02TIG5ydrZDdJOxLzNcH23jyBwLDdDwATZM/tJ2w+fxbHpCCnhxkro0AE2nzWQNGAStGv5jWcoabmGqldR2Ia+5cmPexnHI5OBfdSeohYUNskv+aCrIIVbyIXXNrzJGggSH5P3QVOw==";

TEST(AuthUnittest, Normal) {
  Auth auth;
  auth.Authenticate(TEST_USER, TEST_PWD, [](ErrorPtr err, bool ok) {
    CHECK(err.get() == NULL);
    CHECK(ok);
  });
  auth.Authenticate(TEST_USER, TEST_PWD, [](ErrorPtr err, bool ok) {
    CHECK(err.get() == NULL);
    CHECK(ok);
  });
}

}  // namespace xcomet
