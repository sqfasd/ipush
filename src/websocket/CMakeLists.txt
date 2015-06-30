ADD_LIBRARY(xcomet_websocket
  websocket.cc
  websocket_session.cc
)

TARGET_LINK_LIBRARIES(xcomet_websocket
  xcomet_crypto
)
