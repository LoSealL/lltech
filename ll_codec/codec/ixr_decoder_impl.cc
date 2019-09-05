/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 14th, 2017
changelog
********************************************************************/
#include "ll_codec/codec/ixr_codec_impl.h"

namespace ixr {
DecoderImplIntel::DecoderImplIntel() {}

#ifdef LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H_
DecoderImplIntel::~DecoderImplIntel() { Deallocate(); }

void DecoderImplIntel::Allocate(CodecConfig &config, void *nalu,
                                uint32_t size) {
  m_Object = std::make_unique<mfxvr::dec::CVRDecBase>();
  mfxvr::vrpar::config par{};
  par.codec = config.codec;
  par.multiViewCodec = config.advanced.enableMvc;
  m_bMvc = par.multiViewCodec;
  par.out.color_format = formatConvert(config.outputFormat);
  switch (config.memoryType) {
    case IXR_MEM_INTERNAL_GPU:
      par.renderer = config.device;
      break;
    case IXR_MEM_INTERNAL_CPU:
      par.renderer = nullptr;
      break;
    case IXR_MEM_EXTERNAL_GPU:
    case IXR_MEM_EXTERNAL_CPU:
      // @Todo: TBD...
      break;
  }
  m_Object->Config(&par, static_cast<uint8_t *>(nalu), size);
  config.width = par.in.width;
  config.height = par.in.height;
}

void DecoderImplIntel::Deallocate() {
  void *avoid_inf_loop[2]{};
  do {
    m_Object->DequeueOutputSurface(avoid_inf_loop);
    if (avoid_inf_loop[0]) m_Object->ReleaseOutputSurface(avoid_inf_loop[0]);
  } while (avoid_inf_loop[0]);
  m_Object.reset();
}

CodecStat DecoderImplIntel::GetDecodeStatus() { return CodecStat(); }

int DecoderImplIntel::QueueInputBuffer(void *ptr, uint32_t size) {
  if (!m_Object) return -1;
  return m_Object->QueueInput(ptr, size) ? 0 : -1;
}

int DecoderImplIntel::DequeueOutputBuffer(void **ptr) {
  void *avoid_stack_error[2]{};
  if (m_Object) m_Object->DequeueOutputSurface(avoid_stack_error);
  *ptr = avoid_stack_error[0];
  return *ptr ? 0 : -1;
}

void DecoderImplIntel::ReleaseOutputBuffer(void *ptr) {
  m_Object->ReleaseOutputSurface(ptr);
}

void DecoderImplIntel::GetPrivateData(void *data) const {}

void DecoderImplIntel::SetPrivateData(void *data) {}

uint32_t ixr::DecoderImplIntel::formatConvert(ColorFourcc f) {
  switch (f) {
    case IXR_COLOR_NV12:
      return MFX_FOURCC_NV12;
    case IXR_COLOR_ARGB:
      return MFX_FOURCC_RGB4;
    default:
      break;
  }
  return 0;
}
#endif  // LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H_
}  // namespace ixr
