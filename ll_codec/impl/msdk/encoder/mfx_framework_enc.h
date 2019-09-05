/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  MSDK encoder framework. Define the encoder as a I/O queue and
  core codec.
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 25th, 2017
Mod      :    Date      Author

Updated Vpp. 2017.3.30
********************************************************************/
#ifndef LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H_
#define LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H_
#include <atomic>
#include <memory>
#include <vector>
#include "ll_codec/impl/msdk/encoder/enc_core.h"
#include "ll_codec/impl/msdk/utility/mfx_base.h"
#include "ll_codec/impl/msdk/utility/simplepool.h"


namespace mfxvr {
namespace enc {

class CVRmfxFramework : public CVRmfxSession {
 public:
  explicit CVRmfxFramework(bool hw = true);

  virtual ~CVRmfxFramework();

  void Allocate(const vrpar::config &par, mfxFrameAllocResponse &resp);

  mfxEncodeStat GetEncodeStatus();

  template <class FrameType>
  FrameType DequeueInputBuffer(FrameType p) {
    return reinterpret_cast<FrameType>(dequeueInputBuffer());
  }

  bool QueueInputBuffer();

  bool Run();

  int DequeueOutputBuffer(mfxU8 **pointer, mfxU32 *size);

  mfxBitstream DequeueOutputBuffer();

  void ReleaseOutputBuffer(const mfxBitstream &buf);

  void GetFlowControlParam(mfxF32 *fps, mfxU32 *throughput) const {
    *fps = m_Par.fps;
    *throughput = m_Par.targetKbps;
  }

  void SetFlowControlParam(const mfxF32 &fps, const mfxU32 &throughput) {
    m_Par.fps = fps;
    m_Par.targetKbps = throughput;
  }

 private:  // param
  std::unique_ptr<Core> m_Core;
  std::unique_ptr<SimplePool> m_Pool;
  std::atomic_bool m_bInputLocked;
  // frame surfaces for VPP input
  // may have multiple inputs
  std::vector<mfxFrameSurface1> m_InputSurfaces;
  // I/O index
  mfxU32 m_unIIterator;
  mfxU32 m_unOIterator;
  bool m_bSystemMemory;
  vrpar::config m_Par;
  mfxEncodeCtrl m_Ctrl;
  mfxBitstream m_Output;
  mfxU32 m_BsBufSize;

 private:  // func
  void createAllocator(mfxHDL);

  mfxHDL dequeueInputBuffer();

  mfxU16 adjustQuality(const mfxU16 &unLastQp, const mfxU32 &unLen,
                       const mfxU32 &unMaxLen);
};

}  // namespace enc
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_FRAMEWORK_ENC_H_
