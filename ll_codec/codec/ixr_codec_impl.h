/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 14th, 2017
changelog
********************************************************************/
#ifndef LL_CODEC_CODEC_DETAIL_LL_CODEC_IMPL_H_
#define LL_CODEC_CODEC_DETAIL_LL_CODEC_IMPL_H_
#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include "ll_codec/codec/ixr_codec.h"
#include "ll_codec/codec/ixr_codec_config.h"
#include "ll_codec/impl/thread_safe_stl/queue/thread_safe_queue.h"

namespace ixr {
class EncoderImplIntel : public Encoder {
 public:
  EncoderImplIntel();
#ifdef LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H_
  virtual ~EncoderImplIntel();
  virtual void Allocate(const CodecConfig& config) override;
  virtual void Deallocate() override;
  virtual CodecStat GetEncodeStatus() override;
  virtual void* DequeueInputBuffer() override;
  virtual int QueueInputBuffer(void* ptr) override;
  virtual int QueueUserData(void* data, uint32_t size) override;
  virtual int DequeueUserData(void* data, uint32_t* size) override;
  virtual int DequeueOutputBuffer(void** ptr, uint32_t* size) override;
  virtual void ReleaseOutputBuffer(void* ptr) override;
  virtual void GetFlowControlParam(float* fps,
                                   uint32_t* throughput) const override;
  virtual void SetFlowControlParam(const float fps,
                                   const uint32_t throughput) override;

 protected:
  uint32_t formatConvert(ColorFourcc f);
  uint32_t rcConvert(RateControlMode rc);
  int32_t sliceModeConvert(SliceMode sm);

 private:
  std::unique_ptr<mfxvr::enc::CVRmfxFramework> m_Object;
  std::deque<std::vector<char>> m_UserData;
  std::mutex m_UserMutex;
  bool m_bRunning;
#endif  // LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H
};

class DecoderImplIntel : public Decoder {
 public:
  DecoderImplIntel();
#ifdef LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H_
  virtual ~DecoderImplIntel();
  virtual void Allocate(CodecConfig& config, void* nalu,
                        uint32_t size) override;
  virtual void Deallocate() override;
  virtual CodecStat GetDecodeStatus() override;
  virtual int QueueInputBuffer(void* ptr, uint32_t size) override;
  virtual int DequeueOutputBuffer(void** ptr) override;
  virtual void ReleaseOutputBuffer(void* ptr) override;
  virtual void GetPrivateData(void* data) const override;
  virtual void SetPrivateData(void* data) override;

 protected:
  uint32_t formatConvert(ColorFourcc f);

 private:
  std::unique_ptr<mfxvr::dec::CVRDecBase> m_Object;
  bool m_bMvc;
#endif  // LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H
};

class VppImplIntel :
#ifdef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
    public mfxvr::CVRmfxSession,
#endif
    public Vpp {
 public:
  VppImplIntel();
#ifdef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
  virtual ~VppImplIntel();
  virtual void Allocate(const CodecConfig& config) override;
  virtual void Deallocate() override;
  virtual void* DequeueInputBuffer() override;
  virtual int QueueInputBuffer(void* ptr) override;
  virtual int DequeueOutputBuffer(void** ptr, uint32_t* size) override;
  virtual void ReleaseOutputBuffer(void* ptr) override;

 protected:
  uint32_t formatConvert(ColorFourcc f);
  void createAllocator(const mfxvr::vrpar::config& par);
  // mfxFrameSurface1 *getFreeSurface();

 private:
  bool m_bSystemMemory;
  std::unique_ptr<mfxvr::vpp::VppChain> m_Object;
  std::vector<mfxFrameSurface1> m_InputSurfaces;
  SafeQueue<mfxFrameSurface1*> m_SurfaceFree;
  std::deque<mfxFrameSurface1*> m_SurfaceInUse;
  std::map<mfxHDL, mfxFrameSurface1*> m_TextureSurfaces;
  std::mutex m_MapMutex;
  struct SyncSurface {
    mfxFrameSurface1* surf;
    mfxSyncPoint sync;
  };
  SafeQueue<SyncSurface> m_OutputSurfaces;
#endif  // LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H
};

class EncoderImplNvidia : public Encoder {
 public:
  EncoderImplNvidia();
#ifdef LL_CODEC_NVENC_NV_FRAMEWORK_H_
  virtual ~EncoderImplNvidia();
  virtual void Allocate(const CodecConfig& config) override;
  virtual void Deallocate() override;
  virtual CodecStat GetEncodeStatus() override;
  virtual void* DequeueInputBuffer() override;
  virtual int QueueInputBuffer(void* ptr) override;
  virtual int QueueUserData(void* data, uint32_t size) override;
  virtual int DequeueUserData(void* data, uint32_t* size) override;
  virtual int DequeueOutputBuffer(void** ptr, uint32_t* size) override;
  virtual void ReleaseOutputBuffer(void* ptr) override;
  virtual void GetFlowControlParam(float* fps,
                                   uint32_t* throughput) const override;
  virtual void SetFlowControlParam(const float fps,
                                   const uint32_t throughput) override;

 protected:
  NV_ENC_BUFFER_FORMAT formatConvert(ColorFourcc f);
  NV_ENC_PARAMS_RC_MODE rcConvert(RateControlMode rc);
  uint32_t sliceModeConvert(SliceMode sm);
  std::vector<void*>& allocateInternal(const CodecConfig& config);

 private:
  std::unique_ptr<nvenc::CVRNvFramework> m_Object;
  std::vector<void*> m_MemInternal;
  std::vector<void*>::iterator m_MemIterator;
  std::atomic<size_t> m_InternelMemSize;
  std::deque<std::vector<char>> m_UserData;
  std::mutex m_UserMutex;
  bool m_InternalAllocated;
#endif  // LL_CODEC_NVENC_NV_FRAMEWORK_H
};

}  // namespace ixr
#endif  // LL_CODEC_CODEC_DETAIL_LL_CODEC_IMPL_H_
