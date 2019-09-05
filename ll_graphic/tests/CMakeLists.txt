find_package(GTest REQUIRED)
enable_testing()
include(GoogleTest)

function(ixr_add_test target src ...)
  add_executable(${target} ${src})
  set(ARGS_COUNT 2)
  while(${ARGS_COUNT} LESS ${ARGC})
    list(APPEND linklist ${ARGV${ARGS_COUNT}})
    math(EXPR ARGS_COUNT "${ARGS_COUNT} + 1")
  endwhile()
  target_link_libraries(${target} GTest::GTest GTest::Main ${linklist})
  gtest_discover_tests(${target})
  set_target_properties(${target} PROPERTIES FOLDER "ixr_engine/test")
endfunction(ixr_add_test)

ixr_add_test(test-window test_window.cc ixr_engine)
copy_engine_dependent_to(test-window)

ixr_add_test(test-core test_core.cc ixr_engine)
copy_engine_dependent_to(test-core)

ixr_add_test(test-imshow test_show_image.cc ixr_engine)
copy_engine_dependent_to(test-imshow)

if(IXR_ENGINE_INCLUDE_VULKAN)
  ixr_add_test(test-vulkan test_vulkan.cc ixr_engine)
  copy_engine_dependent_to(test-vulkan)
endif()

ixr_add_test(test-math test_math.cc ixr_engine)
copy_engine_dependent_to(test-math)
ixr_add_test(test-camera test_camera.cc ixr_engine)
copy_engine_dependent_to(test-camera)
