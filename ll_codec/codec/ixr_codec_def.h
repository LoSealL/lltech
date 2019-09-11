/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 14th, 2017
changelog
********************************************************************/
#ifndef LL_CODEC_CODEC_IXR_CODEC_DEF_H_
#define LL_CODEC_CODEC_IXR_CODEC_DEF_H_
#include <stdint.h>
#include <vector>

#ifdef _MSC_VER
// We are using a Microsoft compiler:
#ifdef IXR_CODEC_SHARED_LIBS
#ifdef ixRcodec_EXPORTS
#define IXR_CODEC_API __declspec(dllexport)
#else
#define IXR_CODEC_API __declspec(dllimport)
#endif
#else
#define IXR_CODEC_API
#endif
#else
// Not Microsoft compiler so set empty definition:
#define IXR_CODEC_API
#endif

#ifndef MAKE_FOURCC
#define MAKE_FOURCC(A, B, C, D) \
  ((int)A + (((int)B) << 8) + (((int)C) << 16) + (((int)D) << 24))
#endif

namespace ixr {
enum CodecFourcc {
  IXR_CODEC_AVC = MAKE_FOURCC('A', 'V', 'C', ' '),
  IXR_CODEC_HEVC = MAKE_FOURCC('H', 'E', 'V', 'C'),
  IXR_CODEC_JPEG = MAKE_FOURCC('J', 'P', 'E', 'G')
};

enum ColorFourcc {
  IXR_COLOR_NV12 = MAKE_FOURCC('N', 'V', '1', '2'),
  IXR_COLOR_ARGB = MAKE_FOURCC('A', 'R', 'G', 'B'),
};

//! GPU vendor ID
enum AdapterVendor {
  /** Use Intel Media SDK to encode.
      Require Intel graphic driver (libmfxhw64.dll). */
  IXR_CODEC_VID_INTEL = 0x8086,
  /** Use NVENC to encode.
      Require nvidia driver (nvEncodeAPI64.dll).
      CUDA may be included in future for post-processing. */
  IXR_CODEC_VID_NVIDIA = 0x10DE,
  /** Not implemented yet. */
  IXR_CODEC_VID_AMD = 0x1002,
  /** If use visual studio and its graphic debugger, the default
      d3d device is of this type.
      Buf for encoder, we don't implement for this yet. */
  IXR_CODEC_VID_MSVC_DEBUGGER = 0x1414,
};

//! memory type for input surface
enum MemoryType {
  /** Use external gpu textures as input surfaces.
      The codec doesn't own these resources, those who
      create them should be responsible to release them.
      Users should register those resources when allocate encoder. */
  IXR_MEM_EXTERNAL_GPU,
  /** Use external cpu memories as input surfaces.
      The codec doesn't own these resources, those who
      create them should be responsible to release them.
      Users should register those resources when allocate encoder. */
  IXR_MEM_EXTERNAL_CPU,
  /** Use internal gpu textures as input surfaces.
      The codec owns the resources, and users should call
     Encoder::DequeueInputBuffer to get these resources. */
  IXR_MEM_INTERNAL_GPU,
  /** Use internal cpu memories as input surfaces.
      The codec owns the resources, and users should call
      Encoder::DequeueInputBuffer to get these resources. */
  IXR_MEM_INTERNAL_CPU,
};

//! rate control method
enum RateControlMode {
  //! Auto selected one best method.
  IXR_RC_MODE_AUTO,
  //! Constant QP, should specify QPI and QPP.
  IXR_RC_MODE_CQP,
  //! Constant bitrate, should specify a valid bitrate (bps)
  IXR_RC_MODE_CBR,
  //! Variable bitrate, should specify a max bitrate (bps)
  IXR_RC_MODE_VBR,
};

//! slice mode
enum SliceMode { MB_BASED, BYTE_BASED, TILE_BASED, BLOCK_BASED };

//! Intel Media SDK specific config
struct ConfigGPUSpecificIntel {
  //! Set this to 1 to enable Intel quick sync, using HW to do VPP.
  int32_t enableQsvff : 1;
  /** Set this to 1 to enable encode for region of interest.
      Different regions can encode with different quality. */
  int32_t enableRegionOfInterest : 1;
  //! The number of regions (Maximum 8 regions)
  int32_t numRegions;
  //! Qualities of each region (Maximum 8 regions)
  int32_t regionQP[8];
};

//! NVENC specific config
struct ConfigGPUSpecificNvidia {
  //! Set this to 1 to enable temporal AQ for H.264
  int32_t enableTemporalAQ : 1;
  //! Set this to 1 to enable async operation. May not support old device.
  int32_t enableAsyncMode : 1;
  /** When AQ (Spatial) is enabled, this field is used to specify AQ strength.
      AQ strength scale is from 1 (low) - 15 (aggressive). If not set, strength
      is autoselected by driver.*/
  int32_t aqStrength;
  /** Specifies the maximum bitrate for the encoded output.
      This is used for VBR and ignored for CBR mode. */
  int32_t maxBitrate;
  /** Specifies the VBV(HRD) buffer size. in bits.
      Set 0 to use the default VBV  buffer size. */
  int32_t vbvSize;
};

//! @Todo: TBD...
struct ConfigGPUSpecificAmd {
  int32_t reserved[12];
};

struct CodecStat {
  int32_t numFrames;    //!< current encoded frame index
  int32_t qp;           //!< current encoded quality
  int32_t reserved[8];  //!< reserved bits.
};

struct CodecConfig {
  CodecFourcc codec;   //!< Only H.264/AVC is supported for now
  int32_t width;       //!< Specifies width
  int32_t height;      //!< Specifies height
  int32_t bitrate;     //!< The average bitrate. Will be ignored in CQP mode.
  int32_t constQP[3];  //!< Specifies the QP for I, P, B-frames respectively in
                       //!< CQP mode.
  int32_t fps;         //!< frame per seconds
  int32_t gop;         //!< Specifies the number of pictures in one GOP.
  int32_t asyncDepth;  //!< Specifies depth of output buffer (a ring buffer).
  int32_t outputSizeMax;     //!< The size in byte pre-allocated for each output
                             //!< buffer.
  RateControlMode rcMode;    //!< Specifies rate control mode.
  ColorFourcc inputFormat;   //!< Specifies input color format.
  ColorFourcc outputFormat;  //!< Specifies output color format.
  MemoryType memoryType;     //!< Specifies memory type for input surface
  std::vector<void *>
      sharedMemoryId;     //!< Specifies external memory to be registered
  void *device;           //!< Specifies the D3D11 device handle
  AdapterVendor adapter;  //!< Specifies which gpu to use.
  union {                 //!< Vendor specific config
    ConfigGPUSpecificIntel intel;
    ConfigGPUSpecificNvidia nv;
    ConfigGPUSpecificAmd amd;
  };
  struct advanced {
    int32_t enableSlice : 1;         //!< Set this to 1 to enable slice encode.
    int32_t enableIntraRefresh : 1;  //!< Set this to 1 to enable intra refresh.
    int32_t enableMvc : 1;       //!< Set this to 1 to enable multi-view encode
    SliceMode sliceMode;         //!< Specifies slice mode.
    int32_t sliceData;           //!< Specifies a slice data of that mode.
    int32_t intraRefreshPeriod;  //!< Specifies the interval between successive
                                 //!< intra refresh.
    int32_t intraRefreshDuration;  //!< Specifies the length of intra refresh in
                                   //!< number of frames for periodic intra
                                   //!< refresh. This value should be smaller
                                   //!< than intraRefreshPeriod
  } advanced;
  struct VppConfig {
    int32_t inCrop[4];
    int32_t outWidth;
    int32_t outHeight;
    int32_t outCrop[4];
    // ColorFourcc outFormat;
  } vpp;
};

}  // namespace ixr
#endif  // LL_CODEC_CODEC_IXR_CODEC_DEF_H_
