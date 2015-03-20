SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_EXECUTABLE(unittest.run
  gtest_main.cc
  worker_ut.cc
  remote_storage_ut.cc
  ../../src/worker.cc
  ../../src/event_msgqueue.c
  ../../src/remote_storage.cc
)

TARGET_LINK_LIBRARIES(unittest.run
  pthread
  event
  base
  jsoncpp
  gtest
  ssdb
  ssdb-client
  net
  util
  leveldb
  snappy
)

