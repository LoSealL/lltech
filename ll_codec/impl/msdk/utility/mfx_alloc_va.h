/********************************************************************
Copyright 2016-2018 Tang, Wenyi. All Rights Reserved.

Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 16th, 2017
Mod      :    Date      Author

MSDK external allocator via libva
********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_VA_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_VA_H_
#if !_WIN32
#include <va/va.h>
#include <va/va_android.h>
#include <vector>
#include "ll_codec/impl/msdk/utility/mfx_alloc_base.h"

class CVRVAAllocator : public CMFXAllocator {
 public:
  CVRVAAllocator() = default;

  CVRVAAllocator(VADisplay dpy);

  virtual ~CVRVAAllocator();

 protected:
  virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request,
                                mfxFrameAllocResponse *response);

  virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);

  virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);

  virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

  virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response);

 private:
  VADisplay m_va;

  struct Resource {
    VASurfaceID m_sid;
    uint m_colorformat;
  };
  std::vector<Resource> m_resources;
  std::vector<mfxMemId> m_mid;
  std::vector<mfxFrameAllocResponse> m_resp;
};
#endif
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_VA_H_
