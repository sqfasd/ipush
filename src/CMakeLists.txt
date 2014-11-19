SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ADD_EXECUTABLE(router.server router_server.cc)
TARGET_LINK_LIBRARIES(router.server
  base
  pthread
)


