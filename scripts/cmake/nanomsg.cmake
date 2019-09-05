include(ExternalProject)

set(nanomsg_URL https://github.com/nanomsg/nanomsg.git)
set(nanomsg_TAG 96113e933971f095a44cc2bdcb6694e3976c5d2b) # 1.1.0
set(nanomsg_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/nanomsg/include)

set(cppnanomsg_URL https://github.com/nanomsg/cppnanomsg.git)
set(cppnanomsg_TAG a36d44db1827a36bbd3868825c1b82d23f10e491)

if(WIN32)
  set(nanomsg_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/nanomsg/lib/nanomsg.lib)
  set(nanomsg_SHARED_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/nanomsg/bin/nanomsg.dll)
else(WIN32)
  set(nanomsg_SHARED_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/nanomsg/lib/libnanomsg.so)
endif(WIN32)

ExternalProject_Add(nanomsg-src
  PREFIX nanomsg
  GIT_REPOSITORY ${nanomsg_URL}
  GIT_TAG ${nanomsg_TAG}
  CMAKE_CACHE_ARGS
    -DNN_ENABLE_DOC:BOOL=OFF
    -DNN_ENABLE_GETADDRINFO_A:BOOL=OFF
    -DNN_ENABLE_NANOCAT:BOOL=OFF
    -DNN_STATIC_LIB:BOOL=OFF
    -DNN_TESTS:BOOL=OFF
    -DNN_TOOLS:BOOL=OFF
    -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
    -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
    -DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>)

ExternalProject_Add(cppnanomsg
  PREFIX cppnanomsg
  DEPENDS nanomsg-src
  GIT_REPOSITORY ${cppnanomsg_URL}
  GIT_TAG ${cppnanomsg_TAG}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "" 
  TEST_COMMAND ""
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    <SOURCE_DIR>/nn.hpp ${nanomsg_INCLUDE_DIRS})

# add an imported library for convenient use
include_directories(${nanomsg_INCLUDE_DIRS})
add_library(nanomsg SHARED IMPORTED)
add_dependencies(nanomsg nanomsg-src cppnanomsg)
set_target_properties(nanomsg PROPERTIES
  IMPORTED_LOCATION ${nanomsg_SHARED_LIBS})
if(WIN32)
  set_target_properties(nanomsg PROPERTIES
    IMPORTED_IMPLIB ${nanomsg_LIBS})
endif()

