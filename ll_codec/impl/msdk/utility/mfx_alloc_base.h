/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description
  Intel MediaSDK d3d memory allocator
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_BASE_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_BASE_H_
#include <mfxvideo.h>
#include "ll_codec/impl/msdk/utility/mfx_config.h"

namespace mfxvr {
/**
 * User defined memory allocator
 */
class CMFXAllocator : public mfxFrameAllocator {
 public:
  CMFXAllocator();

  virtual ~CMFXAllocator();

 protected:
  virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request,
                                mfxFrameAllocResponse *response) = 0;

  virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;

  virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;

  virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle) = 0;

  virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response) = 0;

 private:
  static mfxStatus MFX_CDECL Alloc_(mfxHDL pthis, mfxFrameAllocRequest *request,
                                    mfxFrameAllocResponse *response);

  static mfxStatus MFX_CDECL Lock_(mfxHDL pthis, mfxMemId mid,
                                   mfxFrameData *ptr);

  static mfxStatus MFX_CDECL Unlock_(mfxHDL pthis, mfxMemId mid,
                                     mfxFrameData *ptr);

  static mfxStatus MFX_CDECL GetHDL_(mfxHDL pthis, mfxMemId mid,
                                     mfxHDL *handle);

  static mfxStatus MFX_CDECL Free_(mfxHDL pthis,
                                   mfxFrameAllocResponse *response);
};
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_BASE_H_
