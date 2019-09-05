include(ExternalProject)

set(openvr_URL https://github.com/ValveSoftware/openvr.git)
set(openvr_TAG d35c04ca3d7ddb762afa4a1c6c4a9d5de992cda7) # 1.0.13
set(openvr_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/openvr/include)

if(WIN32)
  set(openvr_LIB openvr_api.lib)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM win64)
  else()
    set(PLATFORM win32)
  endif()
  set(openvr_BIN openvr_api.dll)
elseif(APPLE)

elseif(UNIX)

endif()

ExternalProject_Add(openvr-bin
  PREFIX openvr
  GIT_REPOSITORY ${openvr_URL}
  GIT_TAG ${openvr_TAG}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
    <SOURCE_DIR>/headers ${openvr_INCLUDE_DIRS}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    <SOURCE_DIR>/lib/${PLATFORM}/${openvr_LIB} ${CMAKE_CURRENT_BINARY_DIR}/openvr/lib/${openvr_LIB}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    <SOURCE_DIR>/bin/${PLATFORM} ${CMAKE_CURRENT_BINARY_DIR}/openvr/bin)

add_library(openvr INTERFACE)
add_dependencies(openvr openvr-bin)
target_include_directories(openvr INTERFACE ${openvr_INCLUDE_DIRS})
