# Copyright (c) 2019 Tang, Wenyi
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com
file(GLOB RES res/*.*)
foreach(_fd ${RES})
  file(RELATIVE_PATH _rel ${CMAKE_CURRENT_SOURCE_DIR}/res ${_fd})
  if(MSVC)
    file(CREATE_LINK ${_fd} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/${_rel} COPY_ON_ERROR)
    file(CREATE_LINK ${_fd} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/${_rel} COPY_ON_ERROR)
  else()
    file(CREATE_LINK ${_fd} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_rel} COPY_ON_ERROR)
  endif()
endforeach()

discover_all_tests(${CMAKE_CURRENT_SOURCE_DIR} "ll_codec" ixr_codec)
