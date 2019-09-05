# Copyright (c) 2019 Intel Corporation
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com
# FindC4Metal

#[=======================================================================[.rst:
Locate Intel C for Metal developing package

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``CM::RT``
  The C for Metal runtime library, if found


Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``CM_FOUND``
  Found the C for Metal driver
``CM_C_INCLUDE_DIRS``
  the directory containing the cm compiler headers
``CM_RT_INCLUDE_DIRS``
  the directory containing the cm runtime headers

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``CM_ROOT``
  The root directory of the CM driver package distribution (may also be
  set as an environment variable)
``CM_LEGANCY``
  If compiling under WIN32, this variable can be set ``ON`` to enable DX9
  library, ``OFF`` (the default) to enable DX11 library 
#]=======================================================================]

function(__cm_determine_os_arch _var)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(${_var} x64 PARENT_SCOPE)
  else()
    set(${_var} x86 PARENT_SCOPE)
  endif()
endfunction()

__cm_determine_os_arch(CM_OS)
find_file(CM_COMPILER
  NAMES cmc.exe cmc
  HINTS
    ENV CM_ROOT
    ${CM_ROOT}
  DOC "CM kernel compiler (cmc)"
  PATH_SUFFIXES compiler/bin)
find_path(CM_C_INCLUDE_DIR
  cm/cm.h
  HINTS
    $ENV{CM_ROOT}/compiler/include_llvm
    ${CM_ROOT}/compiler/include_llvm
  DOC "CM compiler include dir")
find_path(CM_RT_INCLUDE_DIR
  cm_rt.h
  HINTS
    $ENV{CM_ROOT}/runtime/include
    ${CM_ROOT}/runtime/include
  DOC "CM runtime include dir")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CM DEFAULT_MSG CM_COMPILER CM_C_INCLUDE_DIR CM_RT_INCLUDE_DIR)

if(CM_FOUND)
  include(${CMAKE_CURRENT_LIST_DIR}/c4metal.cmake)
  if(${CM_GEN} STREQUAL "")
    set(CM_GEN $ENV{CM_GEN})
  endif()
  # set(CM_SKU GT2 CACHE STRING "GEN arch name (Default: GEN9)")
  # set(CM_DX_VERSION DX9 CACHE STRING "DX9 | DX11 (Default: DX9)")
  # set(CM_PRODUCT_BRANCH nf CACHE STRING "(Default: nf)")
  # set(CM_RUN_MODE HW CACHE STRING "Execution run mode HW | EMU | SIM (Default: HW)")
  cm_populate_arch_name(${CM_GEN})
  # cm_populate_sku_name(${CM_SKU})
  # cm_populate_dx(${CM_DX_VERSION})
  # cm_populate_mode(${CM_RUN_MODE})
  set(CMAKE_CMC_FLAGS
    -Qxcm
    -Qxcm_jit_target=${GEN_ARCH}
    -mCM_emit_common_isa
    -isystem ${CM_C_INCLUDE_DIR}
    CACHE STRING "cmc flags")
  if(CM_EMU OR CM_SIM)
    # TODO something..
  endif()
  if(NOT TARGET CM::RT)
    add_library(CM::RT INTERFACE IMPORTED)
    set_target_properties(CM::RT PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CM_RT_INCLUDE_DIR}")
    if(CM_LEGANCY)
      cm_populate_dx(CM_DX9)
    else()
      cm_populate_dx(CM_DX11)
    endif()
  endif()  
endif()
mark_as_advanced(CM_COMPILER CM_C_INCLUDE_DIR CM_RT_INCLUDE_DIR CMAKE_CMC_FLAGS)
