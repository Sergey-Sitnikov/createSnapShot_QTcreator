# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "MinSizeRel")
  file(REMOVE_RECURSE
  "CMakeFiles/Screen_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/Screen_autogen.dir/ParseCache.txt"
  "Screen_autogen"
  )
endif()
