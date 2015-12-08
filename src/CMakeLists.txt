SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_SUBDIRECTORY(auth)
ADD_SUBDIRECTORY(storage)
ADD_SUBDIRECTORY(peer)
ADD_SUBDIRECTORY(websocket)
ADD_SUBDIRECTORY(crypto)

ADD_LIBRARY(ipush_core
  http_session.cc
  user.cc
  event_msgqueue.c
  worker.cc
  utils.cc
  stats_manager.cc
  loop_executor.cc
  http_client.cc
)

TARGET_LINK_LIBRARIES(ipush_core
  base
  event
  jsoncpp
)

ADD_EXECUTABLE(ipush_server
  session_server.cc
  main.cc
)
TARGET_LINK_LIBRARIES(ipush_server
  ipush_auth
  ipush_storage
  ipush_peer
  ipush_websocket
  ipush_core
)
