ADD_LIBRARY(ipush_websocket
  websocket.cc
  websocket_session.cc
)

TARGET_LINK_LIBRARIES(ipush_websocket
  ipush_crypto
)
