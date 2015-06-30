ADD_LIBRARY(xcomet_auth
  auth.cc
  mongo_client.cc
)

TARGET_LINK_LIBRARIES(xcomet_auth
  crypto
  mongoc-1.0
  bson
  xcomet_crypto
)
