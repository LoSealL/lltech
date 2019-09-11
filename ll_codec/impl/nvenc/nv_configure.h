/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description : NV configure struct
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 11th, 2017
changelog
********************************************************************/
#ifndef LL_CODEC_NVENC_NV_CONFIGURE_H_
#define LL_CODEC_NVENC_NV_CONFIGURE_H_
#include <stdint.h>
#include <vector>
#include "ll_codec/impl/nvenc/api/nvEncodeAPI++.h"

#ifndef MAKEFOURCC
#define MAKEFOURCC(A, B, C, D) \
  ((((int)A)) + (((int)B) << 8) + (((int)C) << 16) + (((int)D) << 24))
#endif

#ifdef _MSC_VER
// We are using a Microsoft compiler:
#ifdef NVENC_SHARED
#ifdef nvEnc_EXPORTS
#define NV_ENC_API __declspec(dllexport)
#else
#define NV_ENC_API __declspec(dllimport)
#endif
#else
#define NV_ENC_API
#endif

#else
// Not Microsoft compiler so set empty definition:
#define NV_ENC_API
#endif
namespace nvenc {

enum NV_CODEC_FOURCC {
  NV_ENC_CODEC_H264 = MAKEFOURCC('A', 'V', 'C', ' '),
  NV_ENC_CODEC_HEVC = MAKEFOURCC('H', 'E', 'V', 'C'),
};

enum NV_SLICE_MODE {
  NV_ENC_MB_BASED_SLICE = 0,
  NV_ENC_BYTE_BASED_SLICE = 1,
  NV_ENC_MB_ROW_BASED_SLICE = 2,
  NV_ENC_NUM_SLICES_IN_PIC = 3
};

struct EncodeConfig {
  int width;
  int height;
  int fps;
  int codec;
  int gopLength;
  int rcMode;
  int bitrate;
  int constQP[3];
  int vbvMaxBitrate;
  int vbvSize;
  void *deviceHandle;
  int sliceMode;
  int sliceData;
  int asyncDepth;
  int outputBufferSize;
  NV_ENC_BUFFER_FORMAT inputFormat;
  std::vector<NV_ENC_INPUT_PTR> sharedTextures;
  int intraRefreshPeriod;
  int intraRefreshDuration;
  int enableSliceMode : 1;
  int enableAsyncMode : 1;
  int enableTemporalAQ : 1;
  int enableIntraRefresh : 1;
};
}  // namespace nvenc
#endif  // LL_CODEC_NVENC_NV_CONFIGURE_H_
