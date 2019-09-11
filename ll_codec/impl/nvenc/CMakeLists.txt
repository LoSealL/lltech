# Copyright (c) 2019 Tang, Wenyi
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com

# sub-options
option(IXR_CODEC_NVENC_USING_CUDA "Use Cuda to convert color space" OFF)

if(IXR_CODEC_NVENC_USING_CUDA)
  find_package(CUDA REQUIRED)
endif()

set(API
  api/nvEncodeAPI++.h
  nv_framework.h
  nv_error.h
  nv_configure.h)

set(SRC
  src/nv_framework.cc)

if(WIN32)
  # link d3d11 and dxgi in windows
  list(APPEND LINK_LIBS "d3d11.lib" "dxgi.lib")
endif()

if(FOUND_CUDA)
  # do something
endif()

add_library(nvenc OBJECT ${API} ${SRC})
target_link_libraries(nvenc PUBLIC ${LINK_LIBS})
set_target_properties(nvenc PROPERTIES FOLDER "ll_codec/nvenc")
set(NV_SRC $<TARGET_OBJECTS:nvenc> PARENT_SCOPE)
