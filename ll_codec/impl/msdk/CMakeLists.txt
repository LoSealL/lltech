# Copyright (c) 2019 Intel Corporation
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com

set(MSDK_COMPONENTS decoder encoder mfx_dispatch utility vpp)
foreach(_c ${MSDK_COMPONENTS})
  file(GLOB_RECURSE ${_c}_SRC ${_c}/*.cc ${_c}/*.cpp)
  add_library(msdk_${_c} OBJECT ${${_c}_SRC})
  target_include_directories(msdk_${_c} PRIVATE mfx)
  list(APPEND MSDK_SRC $<TARGET_OBJECTS:msdk_${_c}>)
  set_target_properties(msdk_${_c} PROPERTIES FOLDER "ll_codec/msdk")
endforeach()
target_include_directories(msdk_mfx_dispatch PRIVATE mfx_dispatch/include)
target_include_directories(msdk_mfx_dispatch PUBLIC mfx)
set(MSDK_SRC ${MSDK_SRC} PARENT_SCOPE)
