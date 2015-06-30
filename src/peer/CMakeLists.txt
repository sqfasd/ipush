ADD_LIBRARY(xcomet_peer
  peer.cc
)

TARGET_LINK_LIBRARIES(xcomet_peer
  zmq
)
