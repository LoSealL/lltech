include(ExternalProject)

set(glm_URL https://github.com/g-truc/glm.git)
set(glm_TAG 6fa203eeb7fbcbb6f620501fad40359c8a456049) # 0.9.8.5
set(glm_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/glm/include)

ExternalProject_Add(glm-src
  PREFIX glm
  GIT_REPOSITORY ${glm_URL}
  GIT_TAG ${glm_TAG}
  GIT_SHALLOW 1
  CMAKE_CACHE_ARGS
    -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
    -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
    -DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>)

include_directories(${glm_INCLUDE_DIRS})
add_library(glm INTERFACE)
add_dependencies(glm glm-src)
