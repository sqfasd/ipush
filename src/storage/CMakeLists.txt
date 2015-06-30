ADD_LIBRARY(xcomet_storage
  storage.cc
  inmemory_storage.cc
  cassandra_storage.cc
)

TARGET_LINK_LIBRARIES(xcomet_storage
  cassandra_static
  uv
)
