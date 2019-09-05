/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  Intel MediaSDK d3d memory allocator
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_D3D_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_D3D_H_
#include <mfxvideo++.h>
#include "ll_codec/impl/msdk/utility/mfx_alloc_base.h"
#include "ll_codec/impl/msdk/utility/mfx_base.h"
#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
#include <mutex>
#include <vector>


using Microsoft::WRL::ComPtr;

namespace mfxvr {
/* surface resource */
struct resource {
  ComPtr<ID3D11Texture2D> texture;
  ComPtr<ID3D11Texture2D> staging;
  ComPtr<ID3D11Buffer> buffer;
};

const mfxU32 kMaxMidSize = 128;

class CVRDX11Allocator : public CMFXAllocator {
 public:
  CVRDX11Allocator() = default;

  explicit CVRDX11Allocator(mfxHDL hdl, bool lock_to_read = true);

  virtual ~CVRDX11Allocator();

 protected:
  virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request,
                                mfxFrameAllocResponse *response);

  virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);

  virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);

  virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

  virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response);

 protected:
  bool m_lock2read;
  ComPtr<ID3D11Device> m_dxhdl;
  ComPtr<ID3DBlob> m_blob;
  std::mutex m_lock;
  std::vector<resource> m_resources;
  std::vector<mfxMemId> m_mid;
  std::vector<mfxFrameAllocResponse> m_resp;
};
}  // namespace mfxvr
#endif  // WIN32
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_ALLOC_D3D_H_
