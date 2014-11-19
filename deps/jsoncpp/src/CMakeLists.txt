SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(jsoncpp STATIC
  json_reader.cpp
  json_value.cpp
  json_writer.cpp
)
