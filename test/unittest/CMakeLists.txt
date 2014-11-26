SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_EXECUTABLE(unittest.run
  gtest_main.cc
  worker_ut.cc
  storage_ut.cc
  ../../src/worker.cc
  ../../src/event_msgqueue.c
  ../../src/storage.cc
)

TARGET_LINK_LIBRARIES(unittest.run
  pthread
  base
  event
  gtest
  ssdb
  net
  util
  leveldb
  snappy
)

