SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_EXECUTABLE(session_server
  session.cc
  user.cc
  router_proxy.cc
  channel.cc
  storage.cc
  session_server.cc
)
TARGET_LINK_LIBRARIES(session_server
  base
  pthread
  event
)

ADD_EXECUTABLE(router_server 
  router_server.cc
  session_sub_client.cc
  session_pub_client.cc
)
TARGET_LINK_LIBRARIES(router_server
  base
  pthread
  event
  jsoncpp
)
