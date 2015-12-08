ADD_LIBRARY(ipush_storage
  storage.cc
  inmemory_storage.cc
  cassandra_storage.cc
)

TARGET_LINK_LIBRARIES(ipush_storage
  cassandra_static
  uv
)
