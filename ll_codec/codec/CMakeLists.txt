# Copyright (c) 2019 Intel Corporation
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com

configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/ixr_codec_config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/ixr_codec_config.h)

file(GLOB HEADER ./*.h)
file(GLOB DETAIL ./*.cc)

if(IXR_CODEC_SHARED_LIBS)
  set(LIB_TYPE SHARED)
  add_definitions(-DIXR_CODEC_SHARED_LIBS)
else()
  set(LIB_TYPE STATIC)
endif()

if(IXR_CODEC_BUILD_MSDK)
  list(APPEND libcodec msdk_mfx_dispatch)
  list(APPEND DETAIL ${MSDK_SRC})
endif()
if(IXR_CODEC_BUILD_NVENC)
  list(APPEND libcodec nvenc)
  list(APPEND DETAIL ${NV_SRC})
endif()

add_library(ixr_codec ${LIB_TYPE} ${HEADER} ${DETAIL})
target_link_libraries(ixr_codec PUBLIC ${libcodec})
set_target_properties(ixr_codec PROPERTIES FOLDER "ll_codec")

install(TARGETS ixr_codec ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS ixr_codec RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
