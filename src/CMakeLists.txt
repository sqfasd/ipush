SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_SUBDIRECTORY(auth)
ADD_SUBDIRECTORY(storage)
ADD_SUBDIRECTORY(peer)
ADD_SUBDIRECTORY(websocket)
ADD_SUBDIRECTORY(crypto)

ADD_LIBRARY(xcomet_core
  http_session.cc
  user.cc
  event_msgqueue.c
  worker.cc
  utils.cc
  stats_manager.cc
  loop_executor.cc
)

TARGET_LINK_LIBRARIES(xcomet_core
  base
  event
  jsoncpp
)

ADD_EXECUTABLE(xcomet_server
  session_server.cc
  main.cc
)
TARGET_LINK_LIBRARIES(xcomet_server
  xcomet_auth
  xcomet_storage
  xcomet_peer
  xcomet_websocket
  xcomet_core
)
