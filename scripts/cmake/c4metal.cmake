# Copyright (c) 2019 Intel Corporation
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com

# add a cm kernel source to a custom target
# @param target: target name
# @param file: a single kernel source file
function(cmc_add_target target file)
  if(NOT EXISTS ${file})
    set(file ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endif()
  # get file stem: /foo/bar/abc.cxx := abc
  string(REGEX REPLACE "^.*/|^.*\\\\" "" stem ${file})
  string(REGEX REPLACE ".cpp$|.cc$|.cxx$|.c$" "" stem ${stem})
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target})
  add_custom_target(${target}
    COMMAND ${CM_COMPILER} ${file} ${CMAKE_CMC_FLAGS}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${target}/${stem}.isa
    COMMAND_EXPAND_LISTS
    VERBATIM
    SOURCES ${file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${target}/${stem}.isa
    COMMENT "compile kernels")
  set_target_properties(${target} PROPERTIES
    INCLUDE_DIRECTORIES ${CM_C_INCLUDE_DIR}
    RUNTIME_OUTPUT_NAME ${stem}.isa
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}
    FOLDER "CmKernels")
  if(CM_SHOW_DEBUG_MESSAGE)
    message(STATUS "${CM_COMPILER} ${file} -o ${stem}.isa")
  endif(CM_SHOW_DEBUG_MESSAGE)
endfunction()

# Link(copy) cm kernel binary to host target
# @param hostname: host target name
# @param cmname: cm kernel target name. @see cmc_add_target
function(cmc_link_target hostname ...)
  foreach(cmtarget ${ARGV})
    if(${cmtarget} STREQUAL ${hostname})
      continue()
    endif()
    get_target_property(cm_dir ${cmtarget} RUNTIME_OUTPUT_DIRECTORY)
    get_target_property(cm_file ${cmtarget} RUNTIME_OUTPUT_NAME)
    add_custom_command(TARGET ${hostname} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${cm_dir}/${cm_file} $<TARGET_FILE_DIR:${hostname}>
      VERBATIM
      COMMENT "copy isa file")
    add_dependencies(${hostname} ${cmtarget})
  endforeach()
endfunction()

# Include path for given cmc compiler
# @param ...: included path
function(cmc_include_directories ...)
  foreach(p ${ARGV})
    if(EXISTS ${p})
      set(CMAKE_CMC_FLAGS ${CMAKE_CMC_FLAGS} -isystem ${p} PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

# GEN Architecture code name map table
macro(cm_populate_arch_name gen)
  string(TOUPPER ${gen} gen)
  set(GEN_ARCH ${gen})
  set(CM_RT_STEPPING B0)
  if(${gen} STREQUAL GEN7_5)
    set(CM_RT_PLATFORM HSW)
    set(CM_GEN_EMU_ARCH CM_GEN7_5)
  elseif(${gen} STREQUAL GEN8)
    set(CM_RT_PLATFORM BDW)
    set(CM_GEN_EMU_ARCH CM_GEN8)
  elseif(${gen} STREQUAL GEN8_5)
    set(CM_RT_PLATFORM CHV)
    set(CM_GEN_EMU_ARCH CM_GEN8_5)
  elseif(${gen} STREQUAL GEN9)
    set(CM_RT_PLATFORM SKL)
    set(CM_GEN_EMU_ARCH CM_GEN9)
  elseif(${gen} STREQUAL GEN9_3)
    set(GEN_ARCH GLV)
    set(CM_RT_PLATFORM GLV)
    set(CM_GEN_EMU_ARCH CM_GEN9_3)
    set(CM_RT_SKU GT1)
  elseif(${gen} STREQUAL GEN9LP)
    set(CM_RT_PLATFORM BXT)
    set(CM_GEN_EMU_ARCH CM_GEN9LP)
    set(CM_RT_SKU GTA)
  elseif(${gen} STREQUAL GEN9_5)
    set(CM_RT_PLATFORM KBL)
    set(CM_GEN_EMU_ARCH CM_GEN9_5)
  elseif(${gen} STREQUAL GEN9_5LP)
    set(GEN_ARCH BXT)
    set(CM_RT_PLATFORM GLK)
    set(CM_GEN_EMU_ARCH CM_GEN9_5LP)
    set(CM_RT_SKU GT1)
  elseif(${gen} STREQUAL GEN10)
    set(CM_RT_PLATFORM CNL)
    set(CM_GEN_EMU_ARCH CM_GEN10)
  elseif(${gen} STREQUAL GEN11)
    set(CM_RT_PLATFORM ICL)
    set(CM_GEN_EMU_ARCH CM_GEN11)
  elseif(${gen} STREQUAL GEN11LP)
    set(CM_RT_PLATFORM ICLLP)
    set(CM_GEN_EMU_ARCH CM_GEN11LP)
    set(CM_RT_SKU GT1)
  elseif(${gen} STREQUAL GEN12)
    set(CM_RT_PLATFORM ATS)
    set(CM_GEN_EMU_ARCH CM_GEN12)
    set(CM_RT_SKU GT1)
    set(CM_RT_STEPPING A0)
  elseif(${gen} STREQUAL GEN12LP)
    set(CM_RT_PLATFORM TGLLP)
    set(CM_GEN_EMU_ARCH CM_GEN12LP)
    set(CM_RT_SKU GT2)
    set(CM_RT_STEPPING A0)
  elseif(${gen} STREQUAL DG1)
    set(CM_RT_PLATFORM DG1)
    set(CM_GEN_ARCH GEN12LP)
    set(CM_GEN_EMU_ARCH CM_GEN12LP)
    set(CM_RT_SKU GT2)
    set(CM_RT_STEPPING A0)
  else()
    message(FATAL_ERROR "Wrong GEN architecture name: ${gen}!")
  endif()
endmacro(cm_populate_arch_name)

# SKU code name map table
macro(cm_populate_sku_name sku)
  string(TOUPPER ${sku} SKU)
  set(CM_RT_SKU ${SKU})
  if(${SKU} STREQUAL 5X8)
    set(CM_RT_SKU GT2)
  elseif(${SKU} STREQUAL 9X8)
    set(CM_RT_SKU GT3)
  elseif(${SKU} STREQUAL 13X8)
    set(CM_RT_SKU GT4)
  endif()
  if(NOT ${CM_RT_SKU} MATCHES "GT.")
    message(FATAL_ERROR "Wrong SKU name: ${sku}!")
  endif()
endmacro(cm_populate_sku_name)

macro(cm_populate_dx dx)
  string(TOUPPER ${dx} DX)
  set(DX_PREFIX ${DX})
  add_definitions(-D${DX})
endmacro(cm_populate_dx)

macro(cm_populate_mode m)
  string(TOUPPER ${m} M)
  if(${M} STREQUAL EMU)
    set(CM_EMULATE True)
    add_definitions(-DCMRT_EMU -D${CM_GEN_EMU_ARCH})
  elseif(${M} STREQUAL SIM)
    set(CM_SIMULATE True)
    add_definitions(-DCMRT_SIM)
  endif()
endmacro(cm_populate_mode)
