SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(base STATIC
  affinity.cc
  at_exit.cc
  atomicops_internals_x86_gcc.cc
  bloom_filter.cc
  counter.cc
  debug_util.cc
  demangle.cc
  dmg_fp.cc
  env.cc
  flags.cc
  flags_completions.cc
  flags_reporting.cc
  hash.cc
  icu_utf.cc
  logging.cc
  net.cc
  prtime.cc
  safe_strerror_posix.cc
  string16.cc
  string_piece.cc
  string_split.cc
  string_util.cc
  symbolize.cc
  thread.cc
  thread_pool.cc
  time.cc
  utf.cc
  utf_string_conversions.cc
  utf_string_conversion_utils.cc
  file.cc
  file_base.cc
  file_posix.cc
  debug_log.cc
)

TARGET_LINK_LIBRARIES(base pthread rt)
