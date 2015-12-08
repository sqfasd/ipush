ADD_LIBRARY(ipush_peer
  peer.cc
)

TARGET_LINK_LIBRARIES(ipush_peer
  zmq
)
