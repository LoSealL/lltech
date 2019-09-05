/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  Intel MediaSDK d3d memory allocator
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

********************************************************************/
#include "ll_codec/impl/msdk/utility/mfx_alloc_d3d.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <utility>


namespace mfxvr {
CMFXAllocator::CMFXAllocator() {
  pthis = this;
  Alloc = Alloc_;
  Lock = Lock_;
  Free = Free_;
  Unlock = Unlock_;
  GetHDL = GetHDL_;
}

CMFXAllocator::~CMFXAllocator() {}

mfxStatus CMFXAllocator::Alloc_(mfxHDL pthis, mfxFrameAllocRequest *request,
                                mfxFrameAllocResponse *response) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = static_cast<CMFXAllocator *>(pthis);
  return self->AllocFrames(request, response);
}

mfxStatus CMFXAllocator::Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = static_cast<CMFXAllocator *>(pthis);
  return self->LockFrame(mid, ptr);
}

mfxStatus CMFXAllocator::Unlock_(mfxHDL pthis, mfxMemId mid,
                                 mfxFrameData *ptr) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = static_cast<CMFXAllocator *>(pthis);
  return self->UnlockFrame(mid, ptr);
}

mfxStatus CMFXAllocator::GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL *handle) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = static_cast<CMFXAllocator *>(pthis);
  return self->GetFrameHDL(mid, handle);
}

mfxStatus CMFXAllocator::Free_(mfxHDL pthis, mfxFrameAllocResponse *response) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = static_cast<CMFXAllocator *>(pthis);
  return self->FreeFrames(response);
}

#if _WIN32
using MFXFMT = std::pair<mfxU32, DXGI_FORMAT>;

const std::map<mfxU32, DXGI_FORMAT> MFXFormat = {
    MFXFMT(MFX_FOURCC_NV12, DXGI_FORMAT_NV12),
#ifdef EXP_SRGB_CONVERT_INTERNAL
    MFXFMT(MFX_FOURCC_RGB4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB),
#else
    MFXFMT(MFX_FOURCC_RGB4, DXGI_FORMAT_R8G8B8A8_UNORM),
#endif
    MFXFMT(MFX_FOURCC_AYUV, DXGI_FORMAT_AYUV),
    MFXFMT(MFX_FOURCC_YUY2, DXGI_FORMAT_YUY2),
    MFXFMT(MFX_FOURCC_P8, DXGI_FORMAT_P8),  // used for HEVC codec
};

constexpr uint32_t SUPPORTED_TYPE =
    MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_FROM_DECODE |
    MFX_MEMTYPE_FROM_ENCODE;

CVRDX11Allocator::CVRDX11Allocator(mfxHDL hdl, bool lock_to_read) {
  m_mid.reserve(kMaxMidSize);  // need to avoid this vector to recap
  m_dxhdl.Attach(static_cast<ID3D11Device *>(hdl));
  m_lock2read = lock_to_read;
}

CVRDX11Allocator::~CVRDX11Allocator() {
  // Do not release device here
  if (m_dxhdl) m_dxhdl.Detach();
}

mfxStatus CVRDX11Allocator::AllocFrames(mfxFrameAllocRequest *request,
                                        mfxFrameAllocResponse *response) {
  /*
  Allocate internal buffers for VPP processing.
    - Input buffers
    - Intermediate buffers
  Number of buffers are equal to request->suggested
  Responses should be stored for deallocating frames
  */
  using Microsoft::WRL::ComPtr;
  using std::vector;

  // Check inputs
  if (!request || !response) return MFX_ERR_NULL_PTR;

  bool from_dec = (request->Type & MFX_MEMTYPE_FROM_DECODE) != 0;
  // avoid Dec::Init to allocate again
  if (from_dec && !(request->Type & MFX_MEMTYPE_VR_SPECIAL)) {
    *response = m_resp.back();
    return MFX_ERR_NONE;
  }
  if (!(request->Type & SUPPORTED_TYPE)) {
    return MFX_ERR_UNSUPPORTED;
  }
  if (!m_dxhdl) return MFX_ERR_NOT_INITIALIZED;
  size_t idx = 0;
  if (request->Info.FourCC == MFX_FOURCC_P8) {
    D3D11_BUFFER_DESC dc{};
    dc.BindFlags = 0;
    dc.ByteWidth = request->Info.Width * request->Info.Height;
    dc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    dc.MiscFlags = 0;
    dc.StructureByteStride = 0;
    dc.Usage = D3D11_USAGE_STAGING;

    m_resources.push_back(resource());
    auto hr = m_dxhdl->CreateBuffer(&dc, nullptr,
                                    m_resources.back().buffer.GetAddressOf());
    if (FAILED(hr)) return MFX_ERR_MEMORY_ALLOC;
    idx = m_mid.size();
  } else {
    D3D11_TEXTURE2D_DESC dc{};
    dc.ArraySize = 1;
    dc.CPUAccessFlags = 0;
    dc.Format = MFXFormat.at(request->Info.FourCC);
    dc.Height = request->Info.Height;
    dc.MipLevels = 1;
    dc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    dc.SampleDesc.Count = 1;
    dc.SampleDesc.Quality = 0;
    dc.Usage = D3D11_USAGE_DEFAULT;
    dc.Width = request->Info.Width;
    dc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (from_dec && (request->Type & MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET))
      dc.BindFlags = D3D11_BIND_DECODER;

    for (int i = 0; i < request->NumFrameSuggested; ++i) {
      m_resources.push_back(resource());
      auto hr =
          m_dxhdl->CreateTexture2D(&dc, nullptr, &m_resources.back().texture);
      if (FAILED(hr)) return MFX_ERR_MEMORY_ALLOC;
    }
    idx = m_mid.size();

    if (request->Type & (MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_FROM_VPPIN)) {
      dc.BindFlags = 0;
      dc.CPUAccessFlags =
          m_lock2read ? D3D11_CPU_ACCESS_READ : D3D11_CPU_ACCESS_WRITE;
      dc.MiscFlags = 0;
      dc.Usage = D3D11_USAGE_STAGING;
      for (int i = 0; i < request->NumFrameSuggested; ++i) {
        auto hr = m_dxhdl->CreateTexture2D(&dc, nullptr,
                                           &m_resources[idx + i].staging);
        if (FAILED(hr)) return MFX_ERR_MEMORY_ALLOC;
      }
    }
  }
  std::generate_n(std::back_inserter(m_mid), request->NumFrameSuggested, [&]() {
    auto ret = m_mid.empty() ? 1 : (size_t)m_mid.back() + 1;
    return (mfxMemId)ret;
  });
  // avoid vector m_mid to recap
  if (m_mid.size() > kMaxMidSize) return MFX_ERR_NOT_ENOUGH_BUFFER;

  response->AllocId = request->AllocId;
  response->NumFrameActual = request->NumFrameSuggested;
  response->mids = &m_mid[idx];

  m_resp.push_back(*response);
  return MFX_ERR_NONE;
}

mfxStatus CVRDX11Allocator::LockFrame(mfxMemId mid, mfxFrameData *ptr) {
  size_t idx = (size_t)mid - 1;
  if (idx >= m_resources.size() || !ptr) return MFX_ERR_INVALID_VIDEO_PARAM;
  ComPtr<ID3D11DeviceContext> ic;
  D3D11_MAPPED_SUBRESOURCE mappeddata = {};
  m_dxhdl->GetImmediateContext(&ic);
  D3D11_TEXTURE2D_DESC dc{};
  if (m_resources[idx].staging.Get()) {
    m_resources[idx].texture->GetDesc(&dc);
    if (m_lock2read) {
      ic->CopyResource(m_resources[idx].staging.Get(),
                       m_resources[idx].texture.Get());
      ic->Map(m_resources[idx].staging.Get(), 0, D3D11_MAP_READ, 0,
              &mappeddata);
    } else {
      ic->Map(m_resources[idx].staging.Get(), 0, D3D11_MAP_WRITE, 0,
              &mappeddata);
    }
  } else if (m_resources[idx].buffer.Get()) {
    ic->Map(m_resources[idx].buffer.Get(), 0, D3D11_MAP_READ,
            D3D11_MAP_FLAG_DO_NOT_WAIT, &mappeddata);
    dc.Format = DXGI_FORMAT_P8;
  }
  switch (dc.Format) {
    case DXGI_FORMAT_NV12: {
      ptr->Y = static_cast<mfxU8 *>(mappeddata.pData);
      ptr->U = static_cast<mfxU8 *>(mappeddata.pData) +
               dc.Height * mappeddata.RowPitch;
      ptr->V = ptr->U + 1;
      ptr->Pitch = static_cast<mfxU16>(mappeddata.RowPitch);
      ptr->PitchHigh = static_cast<mfxU16>(dc.Height);
      break;
    }
    case DXGI_FORMAT_P8: {
      ptr->Pitch = static_cast<mfxU16>(mappeddata.RowPitch);
      ptr->PitchHigh = static_cast<mfxU16>(dc.Height);
      ptr->Y = static_cast<mfxU8 *>(mappeddata.pData);
      ptr->U = 0;
      ptr->V = 0;
      break;
    }
    default:
      return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
  }
  return MFX_ERR_NONE;
}

mfxStatus CVRDX11Allocator::UnlockFrame(mfxMemId mid, mfxFrameData *) {
  size_t idx = (size_t)mid - 1;
  if (idx >= m_resources.size()) return MFX_ERR_INVALID_VIDEO_PARAM;
  ComPtr<ID3D11DeviceContext> ic;
  m_dxhdl->GetImmediateContext(&ic);
  if (m_resources[idx].staging.Get()) {
    ic->Unmap(m_resources[idx].staging.Get(), 0);
    if (!m_lock2read) {
      ic->CopyResource(m_resources[idx].texture.Get(),
                       m_resources[idx].staging.Get());
    }
  } else if (m_resources[idx].buffer.Get()) {
    ic->Unmap(m_resources[idx].buffer.Get(), 0);
  }
  return MFX_ERR_NONE;
}

mfxStatus CVRDX11Allocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle) {
  // Return texture handle according to mid.
  size_t idx = (size_t)mid - 1;
  if (!handle) return MFX_ERR_INVALID_HANDLE;
  if (idx >= m_resources.size()) return MFX_ERR_NOT_ENOUGH_BUFFER;
  /** \note:
   * the handle acquired here must be a mfxHDLPair the mfxHDLPair::first
   * should be the texture handle and the mfxHDLPair::second should be
   * the subresource index. We don't need subresource index, so here is 0x0.
   * If handle is not mfxHDLPair, then process will return DEVICE_FAILED error.
   */
  std::lock_guard<std::mutex> locker(m_lock);
  mfxHDLPair *pair = reinterpret_cast<mfxHDLPair *>(handle);
  pair->first = m_resources[idx].texture.Get();
  if (!pair->first) {
    pair->first = m_resources[idx].buffer.Get();
  }
  pair->second = nullptr;
  return MFX_ERR_NONE;
}

mfxStatus CVRDX11Allocator::FreeFrames(mfxFrameAllocResponse *) {
  // auto free using ComPtr
  return MFX_ERR_NONE;
}
}  // \namespace mfxvr
#endif
