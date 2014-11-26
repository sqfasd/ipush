PROJECT(xcomet)

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/deps/gtest/ ${PROJECT_SOURCE_DIR}/deps/gtest/include
  ${PROJECT_SOURCE_DIR}/deps/
  ${PROJECT_SOURCE_DIR}/deps/jsoncpp/include/
  ${PROJECT_SOURCE_DIR}/deps/ssdb/src
  /usr/local/include/
)

LINK_DIRECTORIES(
  /usr/lib64/mysql
  ${PROJECT_SOURCE_DIR}/deps/ssdb/src/ssdb
  ${PROJECT_SOURCE_DIR}/deps/ssdb/src/net
  ${PROJECT_SOURCE_DIR}/deps/ssdb/src/util
  ${PROJECT_SOURCE_DIR}/deps/ssdb/deps/snappy-1.1.0/.libs
  ${PROJECT_SOURCE_DIR}/deps/ssdb/deps/jemalloc-3.3.1/lib
  ${PROJECT_SOURCE_DIR}/deps/ssdb/deps/leveldb-1.14.0
)

ADD_DEFINITIONS(-DHAVE_NETINET_IN_H)
#ADD_DEFINITIONS(-O3 -std=c++11 -Wall -g)
#ADD_DEFINITIONS(-O3 -Wall -g)
ADD_DEFINITIONS(-Wall -g)

ADD_SUBDIRECTORY(deps)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(tools)

ENABLE_TESTING()
ADD_TEST(NAME unittest.run COMMAND unittest.run)
