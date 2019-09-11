## A bundle of cmake helper functions used for googletest
## Contact Email: wenyi.tang@intel.com
## Date: 2019.9.6
enable_testing()
include(GoogleTest)

if(NOT EXISTS "$ENV{GTEST_ROOT}" AND NOT EXISTS "${GTEST_ROOT}")
  message(FATAL_ERROR "You have to set GTEST_ROOT before enable this option!")
endif()

if(MSVC)
  set(GTEST_MSVC_SEARCH MT)
endif()

find_package(GTest REQUIRED)

if(GTEST_FOUND)
  include_directories(${GTEST_INCLUDE_DIRS})
endif()

# @brief: find and add all tests in a folder
function(discover_all_tests DIR SUBDIR)
  file(GLOB SRCS ${DIR}/*test*.c ${DIR}/*test*.cc ${DIR}/*test*.cpp)
  foreach(src ${SRCS})
    file(RELATIVE_PATH rel ${DIR} ${src})
    string(REGEX REPLACE ".cpp$|.cc$|.cxx$|.c$" "" tgt ${rel})
    set(tgt gtest_${tgt})
    if(CM_SHOW_DEBUG_MESSAGE)
      message("${rel} -> ${tgt}")
    endif()
    add_executable(${tgt} ${src})
    gtest_discover_tests(${tgt})
    set_target_properties(${tgt} PROPERTIES FOLDER "tests/${SUBDIR}")
    target_link_libraries(${tgt} GTest::GTest GTest::Main ${ARGN})
  endforeach()
endfunction()
