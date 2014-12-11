SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_EXECUTABLE(session_server
  session.cc
  user.cc
  router_proxy.cc
  channel.cc
  event_msgqueue.c
  worker.cc
  storage.cc
  session_server.cc
  utils.cc
)
TARGET_LINK_LIBRARIES(session_server
  base
  pthread
  event
  ssdb
  net #ssdb net
  util # ssdb util
  leveldb
  snappy
)

ADD_EXECUTABLE(router_server
  router_server.cc
  session_sub_client.cc
  session_pub_client.cc
  storage.cc
  worker.cc
  event_msgqueue.c
  utils.cc
  http_client.cc
)
TARGET_LINK_LIBRARIES(router_server
  base
  pthread
  event
  jsoncpp
  ssdb
  net #ssdb net
  util # ssdb util
  leveldb
  snappy
)
