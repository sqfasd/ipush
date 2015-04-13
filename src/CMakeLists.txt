SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_EXECUTABLE(session_server
  session.cc
  user.cc
  event_msgqueue.c
  worker.cc
  session_server.cc
  utils.cc
  stats_manager.cc
  loop_executor.cc
  storage.cc
  inmemory_storage.cc
  cassandra_storage.cc
  peer/peer.cc
  auth.cc
)
TARGET_LINK_LIBRARIES(session_server
  base
  pthread
  event
  jsoncpp
  zmq
  crypto
  cassandra_static
  uv
)
