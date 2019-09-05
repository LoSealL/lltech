/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  Jpeg helper functions, copied from MediaSDK sample_decoder
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 24th, 2017
Mod      :    Date      Author

********************************************************************/
#ifndef LL_CODEC_MFXVR_DECODER_DETAIL_JPEG_HELPER_H_
#define LL_CODEC_MFXVR_DECODER_DETAIL_JPEG_HELPER_H_
#include <mfxdefs.h>
#include <mfxstructures.h>
#include <memory>


template <typename Buf_t, typename Length_t>
bool skip(const Buf_t *&buf, Length_t &length, Length_t step) {
  if (length < step) return false;
  buf += step;
  length -= step;
  return true;
}

static mfxStatus MJPEG_AVI_ParsePicStruct(mfxBitstream *bitstream) {
  // check input for consistency
  if (!bitstream->Data || !bitstream->DataLength) return MFX_ERR_MORE_DATA;
  // define JPEG markers
  const mfxU8 APP0_marker[] = {0xFF, 0xE0};
  const mfxU8 SOI_marker[] = {0xFF, 0xD8};
  const mfxU8 AVI1[] = {'A', 'V', 'I', '1'};
  // size of length field in header
  const mfxU8 len_size = 2;
  // size of picstruct field in header
  const mfxU8 picstruct_size = 1;

  mfxU32 length = bitstream->DataLength;
  const mfxU8 *ptr = reinterpret_cast<const mfxU8 *>(bitstream->Data);

  // search for SOI marker
  while ((length >= sizeof(SOI_marker)) &&
         memcmp(ptr, SOI_marker, sizeof(SOI_marker))) {
    skip(ptr, length, (mfxU32)1);
  }

  // skip SOI
  if (!skip(ptr, length, (mfxU32)sizeof(SOI_marker)) ||
      length < sizeof(APP0_marker))
    return MFX_ERR_MORE_DATA;

  // if there is no APP0 marker return
  if (memcmp(ptr, APP0_marker, sizeof(APP0_marker))) {
    bitstream->PicStruct = MFX_PICSTRUCT_UNKNOWN;
    return MFX_ERR_NONE;
  }

  // skip APP0 & length value
  if (!skip(ptr, length, (mfxU32)sizeof(APP0_marker) + len_size) ||
      length < sizeof(AVI1))
    return MFX_ERR_MORE_DATA;

  if (memcmp(ptr, AVI1, sizeof(AVI1))) {
    bitstream->PicStruct = MFX_PICSTRUCT_UNKNOWN;
    return MFX_ERR_NONE;
  }

  // skip 'AVI1'
  if (!skip(ptr, length, (mfxU32)sizeof(AVI1)) || length < picstruct_size)
    return MFX_ERR_MORE_DATA;

  // get PicStruct
  switch (*ptr) {
    case 0:
      bitstream->PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
      break;
    case 1:
      bitstream->PicStruct = MFX_PICSTRUCT_FIELD_TFF;
      break;
    case 2:
      bitstream->PicStruct = MFX_PICSTRUCT_FIELD_BFF;
      break;
    default:
      bitstream->PicStruct = MFX_PICSTRUCT_UNKNOWN;
  }

  return MFX_ERR_NONE;
}

#endif  // LL_CODEC_MFXVR_DECODER_DETAIL_JPEG_HELPER_H_
