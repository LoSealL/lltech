find_package(GTest REQUIRED)
enable_testing()
include(GoogleTest)

# Add a ctest target
# @param target is the target name
# @param src is the mono test source file
# @param ... are link libraries if any
function(ixr_add_test target src ...)
  add_executable(${target} ${src})
  set(ARGS_COUNT 2)
  while(${ARGS_COUNT} LESS ${ARGC})
    list(APPEND linklist ${ARGV${ARGS_COUNT}})
    math(EXPR ARGS_COUNT "${ARGS_COUNT} + 1")
  endwhile()
  target_link_libraries(${target} GTest::GTest GTest::Main ${linklist})
  gtest_discover_tests(${target})
  set_target_properties(${target} PROPERTIES FOLDER "ll_codec/test")
endfunction(ixr_add_test)

# unit test for ixr_codec
ixr_add_test(test_encode test_encode.cc ixr_codec)
ixr_add_test(test_decode test_decode.cc ixr_codec)
ixr_add_test(test_vpp test_vpp.cc ixr_codec)

file(GLOB RES res/*.*)
foreach(_fd ${RES})
  file(RELATIVE_PATH _rel ${CMAKE_CURRENT_SOURCE_DIR}/res ${_fd})
  if(MSVC)
    file(CREATE_LINK ${_fd} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/${_rel} COPY_ON_ERROR)
    file(CREATE_LINK ${_fd} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/${_rel} COPY_ON_ERROR)
  else()
    file(CREATE_LINK ${_fd} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_rel} COPY_ON_ERROR)
  endif()
endforeach()
