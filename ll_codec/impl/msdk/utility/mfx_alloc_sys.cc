/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  Intel MediaSDK system memory allocator
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    July 2nd, 2017
Mod      :    Date      Author

********************************************************************/
#include "ll_codec/impl/msdk/utility/mfx_alloc_sys.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>


namespace mfxvr {

constexpr mfxU32 SUPPORTED_TYPE = MFX_MEMTYPE_SYSTEM_MEMORY;

CVRSysAllocator::CVRSysAllocator() {
  std::memset(m_mids, 0, sizeof(m_mids));
  m_mid_pos = 0;
}

CVRSysAllocator::~CVRSysAllocator() {
  for (int i = 0; i < kMidLength; i++) {
    if (m_mids[i]) {
      free(m_mids[i]);
      m_mids[i] = nullptr;
    }
  }
}

mfxStatus CVRSysAllocator::AllocFrames(mfxFrameAllocRequest *request,
                                       mfxFrameAllocResponse *response) {
  if (!request || !response) return MFX_ERR_NULL_PTR;
  bool from_dec = (request->Type & MFX_MEMTYPE_FROM_DECODE) != 0;
  // avoid Dec::Init to allocate again
  if (from_dec && !(request->Type & MFX_MEMTYPE_VR_SPECIAL)) {
    *response = m_resp.back();
    return MFX_ERR_NONE;
  }
  if (!(request->Type & SUPPORTED_TYPE)) return MFX_ERR_UNSUPPORTED;
  mfxU32 nbytes = 0;
  mfxU32 Width2 = MAKE_ALIGN32(request->Info.Width);
  mfxU32 Height2 = MAKE_ALIGN32(request->Info.Height);
  switch (request->Info.FourCC) {
    case MFX_FOURCC_YV12:
    case MFX_FOURCC_NV12:
      nbytes = Width2 * Height2 * 3 / 2;
      break;
    case MFX_FOURCC_NV16:
    case MFX_FOURCC_R16:
    case MFX_FOURCC_UYVY:
    case MFX_FOURCC_YUY2:
      nbytes = Width2 * Height2 * 2;
      break;
    case MFX_FOURCC_RGB3:
    case MFX_FOURCC_P010:
      nbytes = Width2 * Height2 * 3;
      break;
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_A2RGB10:  // 4B/P
    case MFX_FOURCC_P210:     // 16bits
      nbytes = Width2 * Height2 * 4;
      break;
    default:
      return MFX_ERR_UNSUPPORTED;
  }
  if (m_mid_pos + request->NumFrameSuggested >= kMidLength)
    return MFX_ERR_NOT_ENOUGH_BUFFER;
  for (int i = 0; i < request->NumFrameSuggested; ++i) {
    mfxU32 head_len = MAKE_ALIGN32(sizeof(systemheader));
    m_mids[m_mid_pos + i] = (mfxMemId)malloc(head_len + nbytes + 32);
    if (!m_mids[m_mid_pos + i]) return MFX_ERR_MEMORY_ALLOC;
    systemheader *head =
        reinterpret_cast<systemheader *>(m_mids[m_mid_pos + i]);
    head->tag = MFX_MEMTAG_SYS;
    head->size = nbytes;
    head->type = request->Type;
    head->info = request->Info;
  }
  response->AllocId = request->AllocId;
  response->NumFrameActual = request->NumFrameSuggested;
  response->mids = &m_mids[m_mid_pos];
  m_mid_pos += response->NumFrameActual;
  m_resp.push_back(*response);
  return MFX_ERR_NONE;
}

mfxStatus CVRSysAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr) {
  bool exist = false;
  for (int i = 0; i < kMidLength; ++i) {
    if (m_mids[i] == mid) {
      exist = true;
      break;
    }
  }
  if (!exist) return MFX_ERR_LOCK_MEMORY;
  if (!ptr) return MFX_ERR_NULL_PTR;
  systemheader *head = reinterpret_cast<systemheader *>(mid);
  if (head->tag != MFX_MEMTAG_SYS) return MFX_ERR_INVALID_HANDLE;

  mfxU16 Width2 = static_cast<mfxU16>(head->info.Width);
  mfxU16 Height2 = static_cast<mfxU16>(head->info.Height);
  ptr->MemId = mid;
  ptr->B = ptr->Y = static_cast<mfxU8 *>(getMemOffset(mid));
  switch (head->info.FourCC) {
    case MFX_FOURCC_NV12:
      ptr->U = ptr->Y + Width2 * Height2;
      ptr->V = ptr->U + 1;
      ptr->Pitch = Width2;
      break;
    case MFX_FOURCC_NV16:
      ptr->U = ptr->Y + Width2 * Height2;
      ptr->V = ptr->U + 1;
      ptr->Pitch = Width2;
      break;
    case MFX_FOURCC_YV12:
      ptr->V = ptr->Y + Width2 * Height2;
      ptr->U = ptr->V + (Width2 >> 1) * (Height2 >> 1);
      ptr->Pitch = Width2;
      break;
    case MFX_FOURCC_UYVY:
      ptr->U = ptr->Y;
      ptr->Y = ptr->U + 1;
      ptr->V = ptr->U + 2;
      ptr->Pitch = 2 * Width2;
      break;
    case MFX_FOURCC_YUY2:
      ptr->U = ptr->Y + 1;
      ptr->V = ptr->Y + 3;
      ptr->Pitch = 2 * Width2;
      break;
    case MFX_FOURCC_RGB3:
      ptr->G = ptr->B + 1;
      ptr->R = ptr->B + 2;
      ptr->Pitch = 3 * Width2;
      break;
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_A2RGB10:
      ptr->G = ptr->B + 1;
      ptr->R = ptr->B + 2;
      ptr->A = ptr->B + 3;
      ptr->Pitch = 4 * Width2;
      break;
    case MFX_FOURCC_R16:
      ptr->Y16 = reinterpret_cast<mfxU16 *>(ptr->B);
      ptr->Pitch = 2 * Width2;
      break;
    case MFX_FOURCC_P010:
      ptr->U = ptr->Y + Width2 * Height2 * 2;
      ptr->V = ptr->U + 2;
      ptr->Pitch = Width2 * 2;
      break;
    case MFX_FOURCC_P210:
      ptr->U = ptr->Y + Width2 * Height2 * 2;
      ptr->V = ptr->U + 2;
      ptr->Pitch = Width2 * 2;
      break;
    case MFX_FOURCC_AYUV:
      ptr->Y = ptr->B;
      ptr->U = ptr->Y + 1;
      ptr->V = ptr->Y + 2;
      ptr->A = ptr->Y + 3;
      ptr->Pitch = 4 * Width2;
      break;
    default:
      return MFX_ERR_UNSUPPORTED;
  }
  // use frameOrder to store format info
  ptr->FrameOrder = head->info.FourCC;
  return MFX_ERR_NONE;
}

mfxStatus CVRSysAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr) {
  return MFX_ERR_NONE;
}

mfxStatus CVRSysAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle) {
  mfxFrameData fr;
  mfxHDLPair *pair = reinterpret_cast<mfxHDLPair *>(handle);
  if (LockFrame(mid, &fr) == MFX_ERR_NONE) {
    switch (fr.FrameOrder) {
      case MFX_FOURCC_RGB3:
      case MFX_FOURCC_RGB4:
      case MFX_FOURCC_A2RGB10:
      case MFX_FOURCC_R16:
        pair->first = fr.B;
        break;
      default:
        pair->first = fr.Y;
        break;
    }
    pair->second = nullptr;
    return MFX_ERR_NONE;
  }
  return MFX_ERR_NOT_FOUND;
}

mfxStatus CVRSysAllocator::FreeFrames(mfxFrameAllocResponse *response) {
  bool exist = false;
  int mid_start = 0;
  if (!response) return MFX_ERR_NULL_PTR;
  for (int i = 0; i < kMidLength; ++i) {
    if (response->mids[0] == m_mids[i]) {
      exist = true;
      mid_start = i;
      break;
    }
  }
  if (!exist) return MFX_ERR_NOT_FOUND;
  for (int i = 0; i < response->NumFrameActual; ++i) {
    if (m_mids[mid_start + i]) free(m_mids[mid_start + i]);
    m_mids[mid_start + i] = nullptr;
  }
  return MFX_ERR_NONE;
}

void *CVRSysAllocator::getMemOffset(mfxMemId mid) {
  return static_cast<mfxU8 *>(mid) + MAKE_ALIGN32(sizeof(systemheader));
}

}  // namespace mfxvr
