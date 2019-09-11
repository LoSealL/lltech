# Copyright (c) 2019 Tang, Wenyi
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com

if(IXR_ENGINE_INCLUDE_DIRECTX11)
  discover_all_tests(${CMAKE_CURRENT_SOURCE_DIR}/dx11 "ixr_engine" ixr_engine)
endif()
if(IXR_ENGINE_INCLUDE_VULKAN)
  discover_all_tests(${CMAKE_CURRENT_SOURCE_DIR}/vk "ixr_engine" ixr_engine)
endif()
discover_all_tests(${CMAKE_CURRENT_SOURCE_DIR}/misc "ixr_engine" ixr_engine)
