SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_EXECUTABLE(unittest
  gtest_main.cc
  message_ut.cc
  worker_ut.cc
  sharding_ut.cc
  peer_ut.cc
  loop_executor_ut.cc
  storage_ut.cc
  auth_ut.cc
  mongo_client_ut.cc
)

TARGET_LINK_LIBRARIES(unittest
  gtest
  xcomet_auth
  xcomet_storage
  xcomet_peer
  xcomet_websocket
  xcomet_core
)
