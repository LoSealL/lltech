/********************************************************************
Copyright 2016-2018 Intel Corp. All Rights Reserved.
Description
  Using MediaSDK in Android as the codec
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 16th, 2017
********************************************************************/
#if !WIN32
#include "ll_codec/impl/msdk/utility/mfx_alloc_va.h"
#include <mfxstructures.h>
#include <map>


using MFXFMT = std::pair<mfxU32, uint>;
const std::map<mfxU32, uint> MFXFormat = {
    MFXFMT(MFX_FOURCC_NV12, VA_FOURCC_NV12),
    MFXFMT(MFX_FOURCC_RGB4, VA_FOURCC_RGBA),
};

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
  auto self = (CMFXAllocator *)pthis;
  return self->AllocFrames(request, response);
}

mfxStatus CMFXAllocator::Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = (CMFXAllocator *)pthis;
  return self->LockFrame(mid, ptr);
}

mfxStatus CMFXAllocator::Unlock_(mfxHDL pthis, mfxMemId mid,
                                 mfxFrameData *ptr) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = (CMFXAllocator *)pthis;
  return self->UnlockFrame(mid, ptr);
}

mfxStatus CMFXAllocator::GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL *handle) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = (CMFXAllocator *)pthis;
  return self->GetFrameHDL(mid, handle);
}

mfxStatus CMFXAllocator::Free_(mfxHDL pthis, mfxFrameAllocResponse *response) {
  if (!pthis) return MFX_ERR_MEMORY_ALLOC;
  auto self = (CMFXAllocator *)pthis;
  return self->FreeFrames(response);
}

CVRVAAllocator::CVRVAAllocator(VADisplay dpy) {
  m_va = dpy;
  m_resources.clear();
}

CVRVAAllocator::~CVRVAAllocator() {
  for (auto &&res : m_resources) {
    vaDestroySurfaces(m_va, &res.m_sid, 1);
  }
}

mfxStatus CVRVAAllocator::AllocFrames(mfxFrameAllocRequest *request,
                                      mfxFrameAllocResponse *response) {
  if (!request || !response) return MFX_ERR_NULL_PTR;
  if (!(request->Type & MFX_MEMTYPE_RESERVED1)) {
    // todo be cautious
    *response = m_resp.front();
    return MFX_ERR_NONE;
  }
  VAStatus va_res;
  uint va_surface_format;
  uint va_rt_format;
  VASurfaceAttrib attrib;
  VASurfaceID *surf;
  mfxU16 surfaces_num = request->NumFrameSuggested;
  if (!surfaces_num) {
    return MFX_ERR_MEMORY_ALLOC;
  }
  // specify surface color format
  std::memset(response, 0, sizeof(mfxFrameAllocResponse));
  va_surface_format = MFXFormat.at(request->Info.FourCC);
  switch (request->Info.FourCC) {
    case MFX_FOURCC_NV12:
      va_rt_format = VA_RT_FORMAT_YUV420;
      break;
    case MFX_FOURCC_RGB4:
      va_rt_format = VA_RT_FORMAT_RGB32;
      break;
    default:
      return MFX_ERR_UNSUPPORTED;
  }
  // create va surfaces
  attrib.type = VASurfaceAttribPixelFormat;
  attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
  attrib.value.type = VAGenericValueTypeInteger;
  attrib.value.value.i = va_surface_format;
  surf = new VASurfaceID[surfaces_num];
  va_res =
      vaCreateSurfaces(m_va, va_rt_format, request->Info.Width,
                       request->Info.Height, surf, surfaces_num, &attrib, 1);
  if (va_res != VA_STATUS_SUCCESS) {
    delete surf;
    return MFX_ERR_MEMORY_ALLOC;
  }
  auto beg = m_mid.size();
  for (mfxU16 i = 0; i < surfaces_num; ++i) {
    m_resources.emplace_back(Resource{surf[i], request->Info.FourCC});
  }
  std::generate_n(std::back_inserter(m_mid), surfaces_num, [&]() {
    auto ret = m_mid.empty() ? 1 : (size_t)m_mid.back() + 1;
    return (mfxMemId)ret;
  });
  response->AllocId = request->AllocId;
  response->mids = &m_mid[beg];
  response->NumFrameActual = surfaces_num;
  // record the response
  m_resp.push_back(*response);
  delete surf;
  return MFX_ERR_NONE;
}

mfxStatus CVRVAAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle) {
  if (mid == 0 || !handle) return MFX_ERR_INVALID_HANDLE;
  auto index = (size_t)mid - 1;
  if (index > m_resources.size()) return MFX_ERR_NOT_ENOUGH_BUFFER;
  *handle = &m_resources[index].m_sid;
  return MFX_ERR_NONE;
}

mfxStatus CVRVAAllocator::FreeFrames(mfxFrameAllocResponse *response) {
  return MFX_ERR_NONE;
}

mfxStatus CVRVAAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr) {
  return MFX_ERR_UNSUPPORTED;
}

mfxStatus CVRVAAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr) {
  return MFX_ERR_UNSUPPORTED;
}
#endif
