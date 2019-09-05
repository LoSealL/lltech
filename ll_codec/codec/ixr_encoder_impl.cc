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
EncoderImplIntel::EncoderImplIntel() {}
#ifdef LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H_
EncoderImplIntel::~EncoderImplIntel() { Deallocate(); }

void EncoderImplIntel::Allocate(const CodecConfig &config) {
  m_Object = std::make_unique<mfxvr::enc::CVRmfxFramework>(true);
  m_bRunning = false;
  mfxvr::vrpar::config par{};
  par.in.cropW = par.in.width = static_cast<uint16_t>(config.width);
  par.in.cropH = par.in.height = static_cast<uint16_t>(config.height);
  par.in.color_format = formatConvert(config.inputFormat);
  par.out = par.in;
  par.out.color_format = MFX_FOURCC_NV12;
  par.targetKbps = config.bitrate;  // the target bitrate has an offset
  par.gop = static_cast<mfxU16>(config.gop);
  par.fps = static_cast<float>(config.fps);
  par.codec = config.codec;
  par.enableQSVFF = config.intel.enableQsvff;
  par.numRoi = static_cast<mfxU16>(config.intel.numRegions);
  for (int i = 0; i < par.numRoi; i++) {
    par.listRoiQPI[i] = static_cast<mfxI16>(config.intel.regionQP[i]);
  }
  par.constQP[0] = config.constQP[0];
  par.constQP[1] = config.constQP[1];
  par.constQP[2] = config.constQP[2];
  par.sliceMode = sliceModeConvert(config.advanced.sliceMode);
  par.sliceData = config.advanced.sliceData;
  par.asyncDepth = config.asyncDepth;
  par.outputSizeMax = config.outputSizeMax;
  par.intraRefresh = config.advanced.enableIntraRefresh;
  par.multiViewCodec = config.advanced.enableMvc;
  par.rateControl = static_cast<uint16_t>(rcConvert(config.rcMode));
  par.slice = static_cast<uint16_t>(config.advanced.sliceData);
  mfxFrameAllocResponse resp{};
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
  m_Object->Allocate(par, resp);
}

void EncoderImplIntel::Deallocate() {
  m_Object.reset();
  m_UserData.~deque();
}

CodecStat EncoderImplIntel::GetEncodeStatus() {
  CodecStat stat;
  auto istat = m_Object->GetEncodeStatus();
  stat.numFrames = istat.NumFrame;
  stat.qp = istat.reserved[0];
  return stat;
}

void *EncoderImplIntel::DequeueInputBuffer() {
  return m_Object->DequeueInputBuffer(mfxHDL(0));
}

int EncoderImplIntel::QueueInputBuffer(void *ptr) {
  return m_Object->QueueInputBuffer() ? 0 : -1;
}

int EncoderImplIntel::QueueUserData(void *data, uint32_t size) {
  std::lock_guard<std::mutex> locker(m_UserMutex);
  m_UserData.emplace_back();
  m_UserData.back().resize(size);
  memcpy(m_UserData.back().data(), data, size);
  return 0;
}

int EncoderImplIntel::DequeueUserData(void *data, uint32_t *size) {
  std::lock_guard<std::mutex> locker(m_UserMutex);
  memcpy(data, m_UserData.front().data(), m_UserData.front().size());
  m_UserData.pop_front();
  return static_cast<int>(m_UserData.size());
}

int EncoderImplIntel::DequeueOutputBuffer(void **ptr, uint32_t *size) {
  if (!m_bRunning) {
    m_bRunning = m_Object->Run();
  }
  int ret = -1;
  if (m_bRunning) {
    ret = m_Object->DequeueOutputBuffer(reinterpret_cast<mfxU8 **>(ptr), size);
  }
  if (ret == 0 || ret != 12) m_bRunning = false;
  return ret;
}

void EncoderImplIntel::ReleaseOutputBuffer(void *ptr) {
  mfxBitstream bs{};
  bs.Data = static_cast<mfxU8 *>(ptr);
  m_Object->ReleaseOutputBuffer(bs);
}

void EncoderImplIntel::GetFlowControlParam(float *fps,
                                           uint32_t *throughput) const {
  m_Object->GetFlowControlParam(fps, throughput);
}

void EncoderImplIntel::SetFlowControlParam(const float fps,
                                           const uint32_t throughput) {
  m_Object->SetFlowControlParam(fps, throughput);
}

uint32_t EncoderImplIntel::formatConvert(ColorFourcc f) {
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

uint32_t EncoderImplIntel::rcConvert(RateControlMode rc) {
  switch (rc) {
    case IXR_RC_MODE_AUTO:
      return mfxvr::MFX_RATECONTROL_AUTO;
    case IXR_RC_MODE_CQP:
      return MFX_RATECONTROL_CQP;
    case IXR_RC_MODE_CBR:
      return MFX_RATECONTROL_CBR;
    case IXR_RC_MODE_VBR:
      return MFX_RATECONTROL_VBR;
  }
  return 0xFFFFFFFF;
}

int32_t EncoderImplIntel::sliceModeConvert(SliceMode sm) {
  switch (sm) {
    case ixr::TILE_BASED:
      return 1;
    case ixr::BLOCK_BASED:
      return 2;
    default:
      break;
  }
  return 3;
}
#endif  // LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H_

EncoderImplNvidia::EncoderImplNvidia() {}

#ifdef LL_CODEC_NVENC_NV_FRAMEWORK_H_
EncoderImplNvidia::~EncoderImplNvidia() { Deallocate(); }

void EncoderImplNvidia::Allocate(const CodecConfig &config) {
  m_Object = std::make_unique<nvenc::CVRNvFramework>();
  m_InternalAllocated = false;
  nvenc::EncodeConfig par{};
  par.width = config.width;
  par.height = config.height;
  par.codec = config.codec;
  par.bitrate = config.bitrate;
  par.constQP[0] = config.constQP[0];
  par.constQP[1] = config.constQP[1];
  par.constQP[2] = config.constQP[2];
  par.fps = config.fps;
  par.gopLength = config.gop;
  par.asyncDepth = config.asyncDepth;
  par.outputBufferSize = config.outputSizeMax;
  par.rcMode = rcConvert(config.rcMode);
  par.inputFormat = formatConvert(config.inputFormat);
  par.deviceHandle = config.device;
  par.enableAsyncMode = config.nv.enableAsyncMode;
  par.enableIntraRefresh = config.advanced.enableIntraRefresh;
  par.enableSliceMode = config.advanced.enableSlice;
  par.enableTemporalAQ = config.nv.enableTemporalAQ;
  par.intraRefreshDuration = config.advanced.intraRefreshDuration;
  par.intraRefreshPeriod = config.advanced.intraRefreshPeriod;
  // par.sliceMode =
  par.sliceData = config.advanced.sliceData;
  par.vbvSize = config.nv.vbvSize;
  par.vbvMaxBitrate = config.nv.maxBitrate;
  switch (config.memoryType) {
    case IXR_MEM_INTERNAL_GPU:
      allocateInternal(config);
      break;
    case IXR_MEM_EXTERNAL_GPU:
      m_MemInternal = config.sharedMemoryId;
      m_MemIterator = m_MemInternal.begin();
      break;
    default:
      break;
  }
  m_InternelMemSize = m_MemInternal.size();
  for (auto &mid : m_MemInternal) {
    par.sharedTextures.push_back(mid);
  }
  m_Object->Allocate(par);
}

void EncoderImplNvidia::Deallocate() {
  m_Object->Deallocate();
  if (m_InternalAllocated) {
    for (auto &ptex : m_MemInternal) {
      ID3D11Texture2D *tex = reinterpret_cast<ID3D11Texture2D *>(ptex);
      tex->Release();
    }
  }
  m_Object.reset();
  m_MemInternal.~vector();
  m_UserData.~deque();
}

CodecStat EncoderImplNvidia::GetEncodeStatus() {
  CodecStat stat{};
  auto nstat = m_Object->GetEncodeStatus();
  stat.numFrames = nstat.picIdx;
  return stat;
}

void *EncoderImplNvidia::DequeueInputBuffer() {
  if (m_MemInternal.empty() || m_InternelMemSize == 0) return nullptr;
  void *p = *m_MemIterator;
  ++m_MemIterator;
  if (m_MemIterator == m_MemInternal.end())
    m_MemIterator = m_MemInternal.begin();
  m_InternelMemSize--;
  return p;
}

int EncoderImplNvidia::QueueInputBuffer(void *ptr) {
  return m_Object->QueueInputBuffer(ptr) ? 0 : -1;
}

int EncoderImplNvidia::QueueUserData(void *data, uint32_t size) {
  std::lock_guard<std::mutex> locker(m_UserMutex);
  m_UserData.emplace_back();
  m_UserData.back().resize(size);
  memcpy(m_UserData.back().data(), data, size);
  return 0;
}

int EncoderImplNvidia::DequeueUserData(void *data, uint32_t *size) {
  std::lock_guard<std::mutex> locker(m_UserMutex);
  memcpy(data, m_UserData.front().data(), m_UserData.front().size());
  m_UserData.pop_front();
  return 0;
}

int EncoderImplNvidia::DequeueOutputBuffer(void **ptr, uint32_t *size) {
  if (m_Object->DequeueOutputBuffer(ptr, size)) {
    m_InternelMemSize++;
    return 0;
  }
  return -1;
}

void EncoderImplNvidia::ReleaseOutputBuffer(void *ptr) {
  m_Object->ReleaseOutputBuffer(ptr);
}

void EncoderImplNvidia::GetFlowControlParam(float *fps,
                                            uint32_t *throughput) const {
  m_Object->GetFlowControlParam(fps, throughput);
}

void EncoderImplNvidia::SetFlowControlParam(const float fps,
                                            const uint32_t throughput) {
  m_Object->SetFlowControlParam(fps, throughput);
}

NV_ENC_BUFFER_FORMAT EncoderImplNvidia::formatConvert(ColorFourcc f) {
  switch (f) {
    case IXR_COLOR_NV12:
      return NV_ENC_BUFFER_FORMAT_NV12;
    case IXR_COLOR_ARGB:
      return NV_ENC_BUFFER_FORMAT_ARGB;
    default:
      break;
  }
  return NV_ENC_BUFFER_FORMAT(0);
}

NV_ENC_PARAMS_RC_MODE EncoderImplNvidia::rcConvert(RateControlMode rc) {
  switch (rc) {
    case IXR_RC_MODE_AUTO:
      break;
    case IXR_RC_MODE_CQP:
      return NV_ENC_PARAMS_RC_CONSTQP;
    case IXR_RC_MODE_CBR:
      return NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
    case IXR_RC_MODE_VBR:
      return NV_ENC_PARAMS_RC_VBR;
    default:
      break;
  }
  return NV_ENC_PARAMS_RC_MODE(0xFFFFFFFF);
}

uint32_t EncoderImplNvidia::sliceModeConvert(SliceMode sm) {
  switch (sm) {
    case ixr::MB_BASED:
      return 0;
    case ixr::BYTE_BASED:
      return 1;
    case ixr::BLOCK_BASED:
      return 3;
    default:
      break;
  }
  return 3;
}

std::vector<void *> &EncoderImplNvidia::allocateInternal(
    const CodecConfig &config) {
  ID3D11Device *dev = reinterpret_cast<ID3D11Device *>(config.device);
  D3D11_TEXTURE2D_DESC dc{};
  dc.Width = config.width;
  dc.Height = config.height;
  dc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  dc.ArraySize = 1;
  dc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  dc.Usage = D3D11_USAGE_DEFAULT;
  dc.MiscFlags = 0;
  dc.CPUAccessFlags = 0;
  dc.MipLevels = 1;
  dc.SampleDesc.Quality = 0;
  dc.SampleDesc.Count = 1;
  for (int i = 0; i < config.asyncDepth; i++) {
    ID3D11Texture2D *tex;
    HRESULT hr = dev->CreateTexture2D(&dc, nullptr, &tex);
    assert(SUCCEEDED(hr));
    m_MemInternal.push_back(tex);
  }
  m_MemIterator = m_MemInternal.begin();
  m_InternalAllocated = true;
  return m_MemInternal;
}
#endif  // LL_CODEC_NVENC_NV_FRAMEWORK_H_
}  // namespace ixr
