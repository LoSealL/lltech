project(ixr_codec VERSION 2.1.8)
# sub-options
option(IXR_CODEC_SHARED_LIBS "Export dynamic library of ixr_codec" OFF)
option(IXR_CODEC_BUILD_MSDK "Building includes Intel Media SDK" ON)
option(IXR_CODEC_BUILD_NVENC "Building includes Nvidia Codec SDK" OFF)
option(IXR_CODEC_BUILD_TESTS "Building unit tests" ON)

if(NOT IXR_CODEC_BUILD_MSDK AND NOT IXR_CODEC_BUILD_NVENC)
  message(FATAL_ERROR "No codec implementation select!")
endif()

if(MSVC)
  enable_compile_flag_mt()
  enable_unicode_charactor()
  set_msvc_warning_level(4)
  ignore_warning(4100 4463)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing -pthread")
endif(MSVC)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
if(IXR_CODEC_BUILD_MSDK)
  add_subdirectory(impl/msdk)
endif()
if(IXR_CODEC_BUILD_NVENC)
  add_subdirectory(impl/nvenc)
endif()
add_subdirectory(codec)  # top class
if(IXR_CODEC_BUILD_TESTS AND LL_BUILD_TESTS)
  add_subdirectory(tests)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h"
        PATTERN "*test*" EXCLUDE)
