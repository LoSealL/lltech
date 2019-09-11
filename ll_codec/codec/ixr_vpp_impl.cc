/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Dec. 6th, 2017
changelog
********************************************************************/
#include "ll_codec/codec/ixr_codec_impl.h"
#ifdef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
#if _WIN32
#include "ll_codec/impl/msdk/utility/mfx_alloc_d3d.h"
#endif
#include "ll_codec/impl/msdk/utility/mfx_alloc_sys.h"
#include "ll_codec/impl/msdk/utility/mfx_error.h"
#endif

namespace ixr {
#ifdef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
using namespace mfxvr;
#endif

VppImplIntel::VppImplIntel() {
#ifdef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
  mfxInitParam initpar{};
  mfxVersion version{0, 1};
  initpar.Version = version;
  initpar.GPUCopy = MFX_GPUCOPY_DEFAULT;
  initpar.Implementation = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D11;
  mfxStatus sts = m_session.InitEx(initpar);
  if (sts != MFX_ERR_NONE) {
    initpar.Implementation = MFX_IMPL_SOFTWARE;
    sts = m_session.InitEx(initpar);
  }
  // API >= 1.18
  mfxVersion ver{18, 1};
  sts = CheckVersion(&ver);
#endif  // LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
}

#ifdef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
VppImplIntel::~VppImplIntel() { Deallocate(); }

void VppImplIntel::Allocate(const CodecConfig& config) {
  mfxvr::vrpar::config par{};
  par.asyncDepth = config.asyncDepth;
  par.renderer = config.device;
  par.in.width = static_cast<mfxU16>(config.width);
  par.in.height = static_cast<mfxU16>(config.height);
  par.in.cropX = static_cast<mfxU16>(config.vpp.inCrop[0]);
  par.in.cropY = static_cast<mfxU16>(config.vpp.inCrop[1]);
  par.in.cropW = static_cast<mfxU16>(config.vpp.inCrop[2]);
  par.in.cropH = static_cast<mfxU16>(config.vpp.inCrop[3]);
  par.in.color_format = formatConvert(config.inputFormat);
  par.out.width = static_cast<mfxU16>(config.vpp.outWidth);
  par.out.height = static_cast<mfxU16>(config.vpp.outHeight);
  par.out.cropX = static_cast<mfxU16>(config.vpp.outCrop[0]);
  par.out.cropY = static_cast<mfxU16>(config.vpp.outCrop[1]);
  par.out.cropW = static_cast<mfxU16>(config.vpp.outCrop[2]);
  par.out.cropH = static_cast<mfxU16>(config.vpp.outCrop[3]);
  par.out.color_format = formatConvert(config.outputFormat);
  m_Object.reset(new mfxvr::vpp::VppChain());
  createAllocator(par);
}

void VppImplIntel::Deallocate() { m_Object.reset(); }

void* VppImplIntel::DequeueInputBuffer() {
  mfxFrameSurface1* free_surface;
  if (m_SurfaceFree.WaitPop(&free_surface)) {
    mfxHDLPair texpair;
    mfxStatus sts = m_allocator->GetHDL(
        m_allocator->pthis, free_surface->Data.MemId, &texpair.first);
    CheckStatus(sts, "- Error in GetHDL", __FILE__, __LINE__);
    std::unique_lock<std::mutex> lock(m_MapMutex);
    m_TextureSurfaces[texpair.first] = free_surface;
    lock.unlock();
    return texpair.first;
  }
  return nullptr;
}

int VppImplIntel::QueueInputBuffer(void* ptr) {
  std::unique_lock<std::mutex> lock(m_MapMutex);
  mfxFrameSurface1* inp = m_TextureSurfaces.at(ptr);
  lock.unlock();
  mfxFrameSurface1* outp;
  mfxSyncPoint sync;
  m_SurfaceInUse.push_back(inp);
  while (!m_SurfaceInUse.empty()) {
    inp = m_SurfaceInUse.front();
    mfxStatus sts = m_Object->RunVpp1(inp, &outp, &sync);
    if (sts == MFX_ERR_NONE) {
      m_OutputSurfaces.Push({outp, sync});
      m_SurfaceInUse.pop_front();
      m_SurfaceFree.Push(inp);
    } else {
      return sts;
    }
  }
  return MFX_ERR_NONE;
}

int VppImplIntel::DequeueOutputBuffer(void** ptr, uint32_t* size) {
  SyncSurface synced_surface;
  if (m_OutputSurfaces.WaitPop(&synced_surface)) {
    mfxSyncPoint sync = synced_surface.sync;
    mfxStatus sts = m_Object->SyncVpp1(sync, MFX_INFINITE);
    mfxHDLPair texpair;
    if (sts == MFX_ERR_NONE) {
      mfxFrameSurface1* surf = synced_surface.surf;
      sts = m_allocator->GetHDL(m_allocator->pthis, surf->Data.MemId,
                                &texpair.first);
      CheckStatus(sts, "- Error in GetHDL", __FILE__, __LINE__);
      *ptr = texpair.first;
      std::unique_lock<std::mutex> lock(m_MapMutex);
      m_TextureSurfaces[*ptr] = surf;
    }
    return sts;
  }
  return MFX_ERR_MORE_DATA;
}

void VppImplIntel::ReleaseOutputBuffer(void* ptr) {
  std::unique_lock<std::mutex> lock(m_MapMutex);
  mfxFrameSurface1* surf = m_TextureSurfaces.at(ptr);
  lock.unlock();
  m_Object->ReleaseSurface(surf);
}

uint32_t VppImplIntel::formatConvert(ColorFourcc f) {
  switch (f) {
    case IXR_COLOR_NV12:
      return MFX_FOURCC_NV12;
    case IXR_COLOR_ARGB:
      return MFX_FOURCC_RGB4;
    default:
      break;
  }
  return 0;
}

void VppImplIntel::createAllocator(const mfxvr::vrpar::config& par) {
  mfxFrameAllocRequest req{};
  mfxFrameAllocResponse resp{};
  mfxStatus sts = MFX_ERR_NONE;
  if (par.renderer) {
#if _WIN32
    m_allocator.reset(new CVRDX11Allocator(par.renderer, false));
#endif
  } else {
    m_allocator.reset(new CVRSysAllocator());
    req.Type = MFX_MEMTYPE_SYSTEM_MEMORY;
  }
  void* hdl = par.renderer;
  if (!hdl) {
    // Use system memory
    m_bSystemMemory = true;
  } else {
#if _WIN32
    // Use video memory
    const mfxHandleType hdl_t = MFX_HANDLE_D3D11_DEVICE;
    if (CVRmfxSession::m_session.SetHandle(hdl_t, hdl) != MFX_ERR_NONE) {
      // Set multiple thread protected for MFX
      ID3D11Device* dxhdl = reinterpret_cast<ID3D11Device*>(hdl);
      ID3D10Multithread* pmt = nullptr;
      dxhdl->QueryInterface(&pmt);
      if (pmt) pmt->SetMultithreadProtected(true);
      sts = CVRmfxSession::m_session.SetHandle(hdl_t, hdl);
    }
    CheckStatus(sts, "- Error in SetHandle", __FILE__, __LINE__);
    sts = CVRmfxSession::m_session.SetFrameAllocator(m_allocator.get());
    CheckStatus(sts, "- Error in SetFrameAllocator", __FILE__, __LINE__);
#endif
  }
  m_Object->Alloc(m_session, m_allocator.get(), par);
  // Alloc shared input buffers
  req.NumFrameMin = req.NumFrameSuggested = static_cast<mfxU16>(par.asyncDepth);
  req.Type |= MFX_MEMTYPE_FROM_VPPIN;
  req.Info.FourCC = par.in.color_format;
  req.Info.Width = par.in.width;
  req.Info.Height = par.in.height;
  CheckStatus(m_allocator->Alloc(m_allocator->pthis, &req, &resp),
              "- Error in Alloc input frames", __FILE__, __LINE__);
  m_InputSurfaces.resize(resp.NumFrameActual);
  mfxU16 i = 0;
  for (auto&& surf : m_InputSurfaces) {
    sts = m_Object->QueryInfo(&surf.Info);
    CheckStatus(sts, "- Vpp::QueryInfo", __FILE__, __LINE__);
    if (par.renderer) {
      surf.Data.MemId = resp.mids[i++];
    } else {
      sts = m_allocator->Lock(m_allocator->pthis, resp.mids[i++], &surf.Data);
      CheckStatus(sts, "- Alloc::Lock", __FILE__, __LINE__);
    }
    m_SurfaceFree.Push(&surf);
  }
}
#endif  // LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
}  // namespace ixr
