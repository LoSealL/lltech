# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
include (ExternalProject)

set(jsoncpp_URL https://github.com/open-source-parsers/jsoncpp.git)
set(jsoncpp_TAG ddabf50f72cf369bf652a95c4d9fe31a1865a781) # 1.8.4
set(jsoncpp_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp/include)

if(WIN32)
  set(jsoncpp_STATIC_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp/lib/jsoncpp.lib)
  set(jsoncpp_STATIC_LIBS_DEBUG
    ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp/lib/jsoncppd.lib)
else()
  set(jsoncpp_STATIC_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp/lib/libjsoncpp.a)
  set(jsoncpp_STATIC_LIBS_DEBUG
    ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp/lib/libjsoncppd.a)
endif()

ExternalProject_Add(jsoncpp-src
  PREFIX jsoncpp
  GIT_REPOSITORY ${jsoncpp_URL}
  GIT_TAG ${jsoncpp_TAG}
  CMAKE_CACHE_ARGS
    -DJSONCPP_WITH_POST_BUILD_UNITTEST:BOOL=OFF
    -DJSONCPP_WITH_TESTS:BOOL=OFF
    -DCMAKE_DEBUG_POSTFIX:STRING=d
    -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
    -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
    -DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>)

include_directories(${jsoncpp_INCLUDE_DIRS})
add_library(jsoncpp STATIC IMPORTED)
add_dependencies(jsoncpp jsoncpp-src)
set_target_properties(jsoncpp PROPERTIES
  IMPORTED_LOCATION ${jsoncpp_STATIC_LIBS}
  IMPORTED_LOCATION_DEBUG ${jsoncpp_STATIC_LIBS_DEBUG})
