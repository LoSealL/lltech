/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description
  Intel MediaSDK system memory allocator
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    July 2nd, 2017
Mod      :    Date      Author

********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_SYS_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_SYS_H_
#include <mfxvideo++.h>
#include <vector>
#include "ll_codec/impl/msdk/utility/mfx_alloc_base.h"
#include "ll_codec/impl/msdk/utility/mfx_base.h"

namespace mfxvr {

enum {
  MFX_MEMTAG_SYS = 0x1116,
  MFX_MEMTAG_ERR = 0xFFFF,
};

struct systemheader {
  mfxU32 tag;
  mfxU32 type;
  mfxU32 size;
  mfxFrameInfo info;
};

constexpr mfxU32 kMidLength = 128;

class CVRSysAllocator : public CMFXAllocator {
 public:
  CVRSysAllocator();

  virtual ~CVRSysAllocator();

 protected:
  virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request,
                                mfxFrameAllocResponse *response);

  virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);

  virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);

  virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

  virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response);

 private:
  void *getMemOffset(mfxMemId mid);

 protected:
  mfxMemId m_mids[kMidLength];
  mfxU16 m_mid_pos;
  mfxU32 m_color_fourcc;
  std::vector<mfxFrameAllocResponse> m_resp;
};
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_SYS_H_
