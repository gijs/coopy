SET(COOPY_VERSION_MAJOR "0")
SET(COOPY_VERSION_MINOR "3")
SET(COOPY_VERSION_PATCH "3")
SET(COOPY_VERSION_MODIFIER "")

configure_file(${CMAKE_SOURCE_DIR}/conf/coopy_version.txt.in
  ${CMAKE_BINARY_DIR}/coopy_version.txt IMMEDIATE)
