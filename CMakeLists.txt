cmake_minimum_required(VERSION 2.6)

project(ipush)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS
  -g
  -DHAVE_NETINET_IN_H    
  -D__STDC_FORMAT_MACROS
  -Wall
  -Wextra
  -Werror
  -Wconversion
)

#set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-limit=1000 -DNDEBUG")

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/deps
  ${PROJECT_SOURCE_DIR}/deps/jsoncpp/include
  ${PROJECT_SOURCE_DIR}/deps/gtest
  ${PROJECT_SOURCE_DIR}/deps/gtest/include
  ${PROJECT_SOURCE_DIR}/deps/libevent
  ${PROJECT_SOURCE_DIR}/deps/libevent/include
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver/src/mongoc
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver/build/src/mongoc
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver/src/libbson/src/bson
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver/build/src/libbson/src/bson
  /usr/local/include
)

link_directories(
  ${PROJECT_SOURCE_DIR}/deps/libevent/.libs
  ${PROJECT_SOURCE_DIR}/deps/cassandra/cpp-driver/build
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver/build/.libs
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver/build/src/libbson/.libs
  /usr/local/lib
  /usr/lib64/mysql
)

add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(tools)

enable_testing()
add_test(NAME unittest.run COMMAND unittest.run)
