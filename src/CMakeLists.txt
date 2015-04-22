SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_LIBRARY(xcomet
  session.cc
  user.cc
  event_msgqueue.c
  worker.cc
  utils.cc
  stats_manager.cc
  loop_executor.cc
  storage.cc
  inmemory_storage.cc
  cassandra_storage.cc
  peer/peer.cc
  auth.cc
  mongo_client.cc
)

TARGET_LINK_LIBRARIES(xcomet
  base
  pthread
  event
  jsoncpp
  zmq
  crypto
  cassandra_static
  uv
  mongoc-1.0
  bson
)

ADD_EXECUTABLE(xcomet_server
  session_server.cc
  main.cc
)
TARGET_LINK_LIBRARIES(xcomet_server
  xcomet
)
