# Copyright (c) 2019
# Author: Wenyi Tang
# E-mail: wenyi.tang@intel.com
# Find LL

#[=======================================================================[.rst:
Locate LL's developing package

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``LL::Codec``
  The codec library, if found
``LL::Graphic``
  The graphic library, if found

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``LL_GRAPHIC_FOUND``
  Found the LL's graphic library
``LL_CODEC_FOUND``
  Found the LL's codec library

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LL_ROOT``
  The root directory of the LL's package distribution (may also be
  set as an environment variable)
#]=======================================================================]

find_path(LL_CODEC_INCLUDE_DIR ll_codec/codec/ixr_codec.h
  HINTS $ENV{LL_ROOT}/include ${LL_ROOT}/include ${LL_DIR}/include
  DOC "LL codec include dir")

find_path(LL_GFX_INCLUDE_DIR ll_graphic/engine/types.h
  HINTS $ENV{LL_ROOT}/include ${LL_ROOT}/include ${LL_DIR}/include
  DOC "LL graphic include dir")

find_library(LL_CODEC_LIBS ixr_codec
  HINTS $ENV{LL_ROOT}/lib ${LL_ROOT}/lib ${LL_DIR}/lib
  DOC "LL codec lib")

find_library(LL_GFX_LIBS ixr_engine
  HINTS $ENV{LL_ROOT}/lib ${LL_ROOT}/lib ${LL_DIR}/lib
  DOC "LL graphic lib")

find_library(LL_CODEC_LIBS_DEBUG ixr_codecd
  HINTS $ENV{LL_ROOT}/lib ${LL_ROOT}/lib ${LL_DIR}/lib
  DOC "LL codec lib")

find_library(LL_GFX_LIBS_DEBUG ixr_engined
  HINTS $ENV{LL_ROOT}/lib ${LL_ROOT}/lib ${LL_DIR}/lib
  DOC "LL graphic lib")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LL_CODEC DEFAULT_MSG LL_CODEC_INCLUDE_DIR LL_CODEC_LIBS)
if(LL_CODEC_FOUND)
  set(MFX_INCLUDE_DIR ${LL_CODEC_INCLUDE_DIR}/ll_codec/impl/msdk/mfx)
  if(NOT TARGET LL::Codec)
    add_library(LL::Codec STATIC IMPORTED)
    set_target_properties(LL::Codec PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LL_CODEC_INCLUDE_DIR};${MFX_INCLUDE_DIR}"
      IMPORTED_LOCATION ${LL_CODEC_LIBS}
      IMPORTED_LOCATION_DEBUG ${LL_CODEC_LIBS_DEBUG})
  endif()
  add_definitions(-DLL_HAS_CODEC)
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(LL_GFX DEFAULT_MSG LL_GFX_INCLUDE_DIR LL_GFX_LIBS)
if(LL_GFX_FOUND)
  if(NOT TARGET LL::Graphic)
    add_library(LL::Graphic STATIC IMPORTED)
    set_target_properties(LL::Graphic PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LL_GFX_INCLUDE_DIR};${LL_GFX_INCLUDE_DIR}/ll_graphic"
      IMPORTED_LOCATION ${LL_GFX_LIBS}
      IMPORTED_LOCATION_DEBUG ${LL_GFX_LIBS_DEBUG})
  endif()
  add_definitions(-DLL_HAS_GFX)
endif()
mark_as_advanced(LL_CODEC_INCLUDE_DIR LL_GFX_INCLUDE_DIR LL_CODEC_LIBS LL_GFX_LIBS)
