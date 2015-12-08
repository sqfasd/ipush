ADD_LIBRARY(ipush_auth
  auth_proxy.cc
  auth_db.cc
  mongo_client.cc
)

TARGET_LINK_LIBRARIES(ipush_auth
  crypto
  mongoc-1.0
  bson
  ipush_crypto
)
