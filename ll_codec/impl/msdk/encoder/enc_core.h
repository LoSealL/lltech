/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description : Intel MediaSDK utilities
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Mar. 24th, 2017
********************************************************************/
#ifndef LL_CODEC_MFXVR_ENCODER_DETAIL_ENC_CORE_H_
#define LL_CODEC_MFXVR_ENCODER_DETAIL_ENC_CORE_H_
#include <memory>
#include <vector>
#include "ll_codec/impl//msdk/vpp/mfx_vpp_chain.h"

namespace mfxvr {
namespace enc {
/**
 * VR encoder core logic.
 */
class Core : public vpp::VppChain {
 public:
  Core(mfxSession s, mfxFrameAllocator *alloc, const vrpar::config &par);

  virtual ~Core();

  /* Run encode async */
  mfxStatus RunEnc(mfxFrameSurface1 *in, mfxBitstream *out,
                   mfxEncodeCtrl *ctrl = nullptr);

  /* Sync encode operation */
  mfxStatus MemorySync(mfxU32 wait);

  /* Query surface information */
  mfxStatus QueryInfo(mfxFrameInfo *info);

 protected:
  // MFX encode API wrapper
  std::unique_ptr<MFXVideoENCODE> m_MfxEnc;
  std::unique_ptr<CENCROI> m_ExtRoi;
  // MFX parameter
  mfxVideoParam m_EncParams;
  mfxU32 m_MvcViews;
  mfxExtCodingOption m_CodingOption;
  mfxExtCodingOption2 m_CodingOption2;
  mfxExtCodingOption3 m_CodingOption3;
  // external buffers
  std::vector<mfxExtBuffer *> m_EncExtBuf;
  std::vector<mfxSyncPoint> m_Sync;

 private:
  mfxStatus initEncParams(const vrpar::config &par);
};

}  // namespace enc
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_ENCODER_DETAIL_ENC_CORE_H_
