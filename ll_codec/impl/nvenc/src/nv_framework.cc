/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description : NV encoder framework
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 11th, 2017
changelog
********************************************************************/
#include "ll_codec/impl/nvenc/nv_framework.h"

namespace nvenc {
struct NV_ENC_BITSTREAM {
  void *pSysmem;
  uint32_t size;
  void *pVmem;  // The memory handle on GPU
  HANDLE sync;
  bool canWrite;
  bool canRead;
};

CVRNvFramework::CVRNvFramework() {
  m_pCore = std::make_unique<CNvEncoder>();
  if (!m_pCore->LoadLibraryAPI()) {
    m_pCore.reset();
    CHECK_STATUS(NV_ENC_ERR_GENERIC, "Failed to load library nvEncodeAPI.dll");
  }
  std::memset(&m_EncodeConfig, 0, sizeof m_EncodeConfig);
  m_EncodeConfig.version = NV_ENC_CONFIG_VER;
  std::memset(&m_EncodeInitPar, 0, sizeof m_EncodeInitPar);
  m_EncodeInitPar.version = NV_ENC_INITIALIZE_PARAMS_VER;
}

CVRNvFramework::~CVRNvFramework() {}

void CVRNvFramework::Allocate(const EncodeConfig &par) {
  NVENCSTATUS sts;
  sts = m_pCore->OpenEncodeSessionEx(par.deviceHandle);
  CHECK_STATUS(sts, "Open encode session");
  initParameters(par);
  sts = m_pCore->InitEncoder(&m_EncodeInitPar);
  CHECK_STATUS(sts, "Initialize encoder");
  allocateIObuffers(par);
  // make a copy of config parameter
  m_Par = par;
}

void CVRNvFramework::Deallocate() { destroyIObuffers(); }

NV_ENC_STAT CVRNvFramework::GetEncodeStatus() {
  NV_ENC_STAT stat{};
  stat.version = NV_ENC_STAT_VER;
  m_pCore->GetEncodeStat(&stat);
  return stat;
}

bool CVRNvFramework::QueueInputBuffer(const HANDLE &tex) {
  NVENCSTATUS sts;
  if (m_CachedRegisteredResources.find(tex) ==
      m_CachedRegisteredResources.end()) {
    // queue an unregistered texture is an error
    CHECK_STATUS(NV_ENC_ERR_RESOURCE_NOT_REGISTERED, "Unregistered texture!");
  }
  NV_ENC_REGISTERED_PTR p = m_CachedRegisteredResources.at(tex);
  // prepare output buffer
  int currentIndex = dequeueOutputIndex();
  if (currentIndex < 0) {
    return false;
  }
  NV_ENC_BITSTREAM &currentBitstream = m_OutputBuffers[currentIndex];
  // map input buffer
  NV_ENC_INPUT_PTR mapped;
  sts = m_pCore->MapResource(p, &mapped);
  CHECK_STATUS(sts, "mapping resources");
  NV_ENC_PIC_PARAMS encodeParams{};
  encodeParams.version = NV_ENC_PIC_PARAMS_VER;
  encodeParams.bufferFmt = m_Par.inputFormat;
  encodeParams.inputBuffer = mapped;
  encodeParams.inputWidth = m_Par.width;
  encodeParams.inputHeight = m_Par.height;
  encodeParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
  encodeParams.outputBitstream = currentBitstream.pVmem;
  encodeParams.completionEvent = currentBitstream.sync;
  sts = m_pCore->EncodeFrame(&encodeParams);
  CHECK_STATUS(sts, "encode frames");
  sts = m_pCore->UnmapResource(mapped);
  CHECK_STATUS(sts, "unmap resources");
  currentBitstream.canWrite = false;
  currentBitstream.canRead = false;
  return sts == NV_ENC_SUCCESS;
}

bool CVRNvFramework::DequeueOutputBuffer(void **ptr, uint32_t *size) {
  int n = m_nOutputRIndex % m_OutputBuffers.size();
  NV_ENC_BITSTREAM *bs = &m_OutputBuffers[n];
  if (bs->canRead || bs->canWrite) return false;
  auto hr = WaitForSingleObject(bs->sync, 100);
  if (FAILED(hr)) return false;
  // Todo: don't know what if lock failed
  if (m_pCore->LockBitstream(bs->pVmem, &bs->pSysmem, &bs->size) ==
      NV_ENC_SUCCESS) {
    bs->canRead = true;
    *ptr = bs->pSysmem;
    *size = bs->size;
    m_CachedVideoMemory[*ptr] = bs;
    m_nOutputRIndex++;
    return true;
  } else {
    return false;
  }
}

void CVRNvFramework::ReleaseOutputBuffer(void *ptr) {
  if (m_CachedVideoMemory.find(ptr) == m_CachedVideoMemory.end()) return;
  NV_ENC_BITSTREAM *bs = m_CachedVideoMemory[ptr];
  NVENCSTATUS sts = m_pCore->UnlockBitstream(bs->pVmem);
  CHECK_STATUS(sts, "Unlock bitstream");
  bs->canWrite = true;
  m_CachedVideoMemory.erase(ptr);
}

GUID CVRNvFramework::getGuidFromFourCC(const uint32_t fourcc) {
  switch (fourcc) {
    case NV_ENC_CODEC_H264:
      return NV_ENC_CODEC_H264_GUID;
    case NV_ENC_CODEC_HEVC:
      return NV_ENC_CODEC_HEVC_GUID;
    default:
      break;
  }
  return GUID();
}

void CVRNvFramework::initParameters(const EncodeConfig &par) {
  NVENCSTATUS sts;
  m_EncodeGuid = getGuidFromFourCC(par.codec);
  sts = m_pCore->GetDefaultConfig(m_EncodeGuid, &m_EncodeConfig);
  CHECK_STATUS(sts, "Get default preset config");
  // fixed params
  m_EncodeInitPar.encodeWidth = par.width;
  m_EncodeInitPar.encodeHeight = par.height;
  m_EncodeInitPar.darWidth = par.width;
  m_EncodeInitPar.darHeight = par.height;
  m_EncodeInitPar.maxEncodeWidth = par.width;
  m_EncodeInitPar.maxEncodeHeight = par.height;
  m_EncodeInitPar.encodeGUID = m_EncodeGuid;
  m_EncodeInitPar.presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
  m_EncodeInitPar.frameRateDen = 1;
  m_EncodeInitPar.frameRateNum = par.fps;
  m_EncodeInitPar.enablePTD = 1;
  m_EncodeInitPar.encodeConfig = &m_EncodeConfig;

  m_EncodeConfig.gopLength = par.gopLength;
  m_EncodeConfig.frameIntervalP = 1;
  m_EncodeConfig.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;
  m_EncodeConfig.mvPrecision = NV_ENC_MV_PRECISION_QUARTER_PEL;
  m_EncodeConfig.monoChromeEncoding = 0;
  // rate control mode
  m_EncodeConfig.rcParams.rateControlMode =
      static_cast<NV_ENC_PARAMS_RC_MODE>(par.rcMode);
  m_EncodeConfig.rcParams.version = NV_ENC_RC_PARAMS_VER;
  switch (m_EncodeConfig.rcParams.rateControlMode) {
    case NV_ENC_PARAMS_RC_CONSTQP:
      m_EncodeConfig.rcParams.constQP.qpIntra = par.constQP[0];
      m_EncodeConfig.rcParams.constQP.qpInterP = par.constQP[1];
      m_EncodeConfig.rcParams.constQP.qpInterB = par.constQP[2];
      break;
    case NV_ENC_PARAMS_RC_VBR:
    case NV_ENC_PARAMS_RC_VBR_HQ:
      m_EncodeConfig.rcParams.maxBitRate = par.vbvMaxBitrate;
      m_EncodeConfig.rcParams.vbvBufferSize = par.vbvSize;
      m_EncodeConfig.rcParams.vbvInitialDelay = 0;
    case NV_ENC_PARAMS_RC_CBR:
    case NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ:
    case NV_ENC_PARAMS_RC_CBR_HQ:
      m_EncodeConfig.rcParams.averageBitRate = par.bitrate;
    default:
      break;
  }
  m_EncodeConfig.rcParams.zeroReorderDelay = 1;
  m_EncodeConfig.rcParams.enableAQ = 1;
  m_EncodeConfig.rcParams.aqStrength = 0;  // auto
  // h264 specific params
  m_EncodeConfig.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
  m_EncodeConfig.encodeCodecConfig.h264Config.idrPeriod = par.gopLength;
  // -- check chroma support
  if (!m_pCore->IsSupportInputFormat(m_EncodeGuid, par.inputFormat)) {
    CHECK_STATUS(NV_ENC_ERR_UNSUPPORTED_PARAM, "Unsupported input format");
  }
  switch (par.inputFormat) {
    case NV_ENC_BUFFER_FORMAT_NV12:
    case NV_ENC_BUFFER_FORMAT_YV12:
    case NV_ENC_BUFFER_FORMAT_IYUV:
      m_EncodeConfig.encodeCodecConfig.h264Config.chromaFormatIDC = 1;
      break;
    case NV_ENC_BUFFER_FORMAT_YUV444:
      if (!m_pCore->IsSupportCapacity(m_EncodeGuid,
                                      NV_ENC_CAPS_SUPPORT_YUV444_ENCODE)) {
        CHECK_STATUS(NV_ENC_ERR_UNSUPPORTED_PARAM,
                     "Don't support YUV444 encode on this device");
      }
    case NV_ENC_BUFFER_FORMAT_ARGB:
    case NV_ENC_BUFFER_FORMAT_ABGR:
    case NV_ENC_BUFFER_FORMAT_AYUV:
      m_EncodeConfig.encodeCodecConfig.h264Config.chromaFormatIDC =
          3;  // Should convert to YUV444
      break;
    default:
      // Unsupported format
      CHECK_STATUS(NV_ENC_ERR_UNSUPPORTED_PARAM, "Unsupported input format");
      break;
  }
  // -- intra refresh
  if (par.enableIntraRefresh) {
    m_EncodeConfig.encodeCodecConfig.h264Config.enableIntraRefresh = 1;
    m_EncodeConfig.encodeCodecConfig.h264Config.intraRefreshCnt =
        par.intraRefreshDuration;
    m_EncodeConfig.encodeCodecConfig.h264Config.intraRefreshPeriod =
        par.intraRefreshPeriod;
  }
  // -- slice encode
  if (par.enableSliceMode) {
    m_EncodeConfig.encodeCodecConfig.h264Config.sliceMode = par.sliceMode;
    m_EncodeConfig.encodeCodecConfig.h264Config.sliceModeData = par.sliceData;
  }
  // async mode
  if (par.enableAsyncMode) {
    if (m_pCore->IsSupportCapacity(m_EncodeGuid,
                                   NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT)) {
      m_EncodeInitPar.enableEncodeAsync = 1;
    }
  }
  if (par.enableTemporalAQ) {
    if (m_pCore->IsSupportCapacity(m_EncodeGuid,
                                   NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ)) {
      m_EncodeConfig.rcParams.enableTemporalAQ = 1;
    }
  }
}

void CVRNvFramework::allocateIObuffers(const EncodeConfig &par) {
  NVENCSTATUS sts;
  // register input buffers
  for (auto &tex : par.sharedTextures) {
    NV_ENC_REGISTERED_PTR p;
    sts = m_pCore->RegisterResource(par.width, par.height, 0, par.inputFormat,
                                    tex, &p);
    CHECK_STATUS(sts, "Register shared textures");
    m_CachedRegisteredResources[tex] = p;
  }
  // allocate output buffers
  m_OutputBuffers.resize(par.asyncDepth);
  for (auto &buf : m_OutputBuffers) {
    buf.size = par.outputBufferSize;
    buf.canWrite = true;
    buf.canRead = false;
    sts = m_pCore->CreateBitstreamBuffer(buf.size, &buf.pVmem);
    CHECK_STATUS(sts, "Create bitstream buffer");
    sts = m_pCore->RegisterSyncEvent(&buf.sync);
    CHECK_STATUS(sts, "Register sync event");
  }
  m_nOutputRIndex = 0;
  m_nOutputWIndex = 0;
}

void CVRNvFramework::destroyIObuffers() {
  NVENCSTATUS sts;
  // destroy 'I' buffers
  for (auto &registered : m_CachedRegisteredResources) {
    sts = m_pCore->UnregisterResource(registered.second);
    CHECK_STATUS(sts, "Unregister resources");
  }
  m_CachedRegisteredResources.clear();
  // destroy 'O' buffers
  for (auto &buf : m_OutputBuffers) {
    sts = m_pCore->UnregisterSyncEvent(buf.sync);
    CHECK_STATUS(sts, "Unregister sync event");
    sts = m_pCore->DestroyBitstreamBuffer(buf.pVmem);
    CHECK_STATUS(sts, "Destroy bitstream buffer");
  }
  m_OutputBuffers.clear();
}

int CVRNvFramework::dequeueOutputIndex() {
  int n = m_nOutputWIndex % m_OutputBuffers.size();
  if (m_OutputBuffers[n].canWrite) return n;
  n = (n + 1) % m_OutputBuffers.size();
  if (!m_OutputBuffers[n].canWrite) return -1;
  m_nOutputWIndex++;
  return n;
}
}  // namespace nvenc
