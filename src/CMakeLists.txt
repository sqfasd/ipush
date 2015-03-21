SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_EXECUTABLE(session_server
  session.cc
  user.cc
  router_proxy.cc
  event_msgqueue.c
  worker.cc
  session_server.cc
  utils.cc
)
TARGET_LINK_LIBRARIES(session_server
  base
  pthread
  event
  jsoncpp
)

ADD_EXECUTABLE(router_server
  remote_storage.cc
  worker.cc
  event_msgqueue.c
  utils.cc
  socketclient.cc
  session_proxy.cc
  router_server.cc
)
TARGET_LINK_LIBRARIES(router_server
  base
  pthread
  event
  jsoncpp
  ssdb
  ssdb-client
  net #ssdb net
  util # ssdb util
  leveldb
  snappy
)
