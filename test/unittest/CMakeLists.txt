SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_EXECUTABLE(unittest.run
  gtest_main.cc
  worker_ut.cc
  sharding_ut.cc
  peer_ut.cc
  loop_executor_ut.cc
  ../../src/worker.cc
  ../../src/event_msgqueue.c
  ../../src/utils.cc
  ../../src/peer/peer.cc
  ../../src/loop_executor.cc
)

TARGET_LINK_LIBRARIES(unittest.run
  pthread
  event
  base
  jsoncpp
  gtest
  zmq
)

