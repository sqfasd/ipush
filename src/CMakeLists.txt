SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#ADD_EXECUTABLE(router_server router_server.cc)
#TARGET_LINK_LIBRARIES(router_server
#  base
#  pthread
#)

ADD_EXECUTABLE(xcomet_server
  xcomet_server.cc
  channel.cc
  user.cc
  storage.cc
  router_proxy.cc
)
TARGET_LINK_LIBRARIES(xcomet_server
  base
  pthread
  event
)
