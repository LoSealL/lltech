include(ExternalProject)

set(ffmpeg_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/include)
set(ffmpeg_LIB_DIRS ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib)
set(ffmpeg_BIN_DIRS ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/bin)

function(add_ff_target tgt ver)
  add_library(${tgt} SHARED IMPORTED)
  if(WIN32)
    add_dependencies(${tgt} ffmpeg-header ffmpeg-dll)
    set_target_properties(${tgt} PROPERTIES
      IMPORTED_LOCATION ${ffmpeg_BIN_DIRS}/${tgt}-${ver}.dll
      IMPORTED_IMPLIB ${ffmpeg_LIB_DIRS}/${tgt}.lib)
  else(WIN32)
    set_target_properties(${tgt} PROPERTIES
      IMPORTED_LOCATION ${ffmpeg_BIN_DIRS}/lib${tgt}-${ver}.so)
  endif(WIN32)
endfunction()

if(WIN32)
  set(ffmpeg_DEV_URL https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-3.4.1-win64-dev.zip)
  set(ffmpeg_BIN_URL https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-3.4.1-win64-shared.zip)
  
  ExternalProject_Add(ffmpeg-header
    PREFIX ffmpeg
    URL ${ffmpeg_DEV_URL}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
      <SOURCE_DIR>/include ${ffmpeg_INCLUDE_DIRS}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      <SOURCE_DIR>/lib ${ffmpeg_LIB_DIRS})
    
  ExternalProject_Add(ffmpeg-dll
    PREFIX ffmpeg
    URL ${ffmpeg_BIN_URL}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
      <SOURCE_DIR>/bin ${ffmpeg_BIN_DIRS})
  
  include_directories(${ffmpeg_INCLUDE_DIRS})
else(WIN32)
  set(ffmpeg_URL https://github.com/FFmpeg/FFmpeg)
  set(ffmpeg_TAG bc839fb39dc376d462856863de2933f0b6b0351a) # 3.4.1
  ExternalProject_Add(ffmpeg
    PREFIX ffmpeg
    GIT_REPOSITORY ${ffmpeg_URL}
    GIT_TAG ${ffmpeg_TAG}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")
endif(WIN32)

add_ff_target(avcodec 57)
add_ff_target(avdevice 57)
add_ff_target(avfilter 6)
add_ff_target(avformat 57)
add_ff_target(avutil 55)
add_ff_target(postproc 54)
add_ff_target(swresample 2)
add_ff_target(swscale 4)
