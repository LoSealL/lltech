/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description
  MSDK encoder framework. Define the encoder as a I/O queue and
  core codec.
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 25th, 2017
Mod      :    Date      Author

Updated Vpp. 2017.3.30
********************************************************************/
#include "ll_codec/impl/msdk/encoder/mfx_framework_enc.h"
#include <cmath>
#include <memory>
#if _WIN32
#include "ll_codec/impl/msdk/utility/mfx_alloc_d3d.h"
#endif
#include "ll_codec/impl/msdk/utility/mfx_alloc_sys.h"

namespace mfxvr {
namespace enc {

CVRmfxFramework::CVRmfxFramework(bool hw) {
  mfxInitParam initpar{};
  mfxVersion version{0, 1};
  initpar.Version = version;
  initpar.GPUCopy = MFX_GPUCOPY_DEFAULT;
  initpar.Implementation = MFX_IMPL_HARDWARE_ANY;
  if (hw) initpar.Implementation |= MFX_IMPL_VIA_D3D11;
  mfxStatus sts = m_session.InitEx(initpar);
  if (sts != MFX_ERR_NONE) {
    initpar.Implementation = MFX_IMPL_SOFTWARE;
    sts = m_session.InitEx(initpar);
  }
  CheckStatus(sts, "- Error in Session::InitEx", __FILE__, __LINE__);
  // API >= 1.18
  mfxVersion ver{18, 1};
  sts = CheckVersion(&ver);
  CheckStatus(sts, MFX_ERR_NONE,
              "- Version unsupported: ver %d.%d, required 1.18", ver.Major,
              ver.Minor);
  m_bSystemMemory = false;
  m_bInputLocked = false;
}

CVRmfxFramework::~CVRmfxFramework() { m_InputSurfaces.clear(); }

void CVRmfxFramework::Allocate(const vrpar::config &par,
                               mfxFrameAllocResponse &resp) {
  m_Par = par;
  std::memset(&m_Ctrl, 0, sizeof(m_Ctrl));
  if (!m_allocator) {
    mfxFrameAllocRequest req{};
    if (par.renderer) {
#if _WIN32      
      m_allocator.reset(new CVRDX11Allocator(par.renderer, false));
#endif
    } else {
      m_allocator.reset(new CVRSysAllocator());
      req.Type = MFX_MEMTYPE_SYSTEM_MEMORY;
    }
    // Alloc shared input buffers
    req.NumFrameMin = req.NumFrameSuggested =
        static_cast<mfxU16>(par.asyncDepth);
    req.Type |= MFX_MEMTYPE_FROM_VPPIN;
    req.Info.FourCC = par.in.color_format;
    req.Info.Width = par.in.width;
    req.Info.Height = par.in.height;
    CheckStatus(m_allocator->Alloc(m_allocator->pthis, &req, &resp),
                "- Error in Alloc input frames", __FILE__, __LINE__);
  }
  createAllocator(par.renderer);
  m_Core = std::make_unique<Core>(m_session, m_allocator.get(), par);
  m_InputSurfaces.resize(resp.NumFrameActual);
  mfxStatus sts;
  mfxU16 i = 0;
  for (auto &&surf : m_InputSurfaces) {
    sts = m_Core->QueryInfo(&surf.Info);
    CheckStatus(sts, "- Core::QueryInfo", __FILE__, __LINE__);
    if (par.renderer) {
      surf.Data.MemId = resp.mids[i++];
    } else {
      sts = m_allocator->Lock(m_allocator->pthis, resp.mids[i++], &surf.Data);
      CheckStatus(sts, "- Alloc::Lock", __FILE__, __LINE__);
    }
  }
  m_unIIterator = 0;
  m_unOIterator = 0;
  m_BsBufSize = par.outputSizeMax;
  m_Pool = std::make_unique<SimplePool>(resp.NumFrameActual * m_BsBufSize +
                                        m_BsBufSize);
}

mfxEncodeStat CVRmfxFramework::GetEncodeStatus() {
  mfxEncodeStat stat{};
  auto sts = MFXVideoENCODE_GetEncodeStat(m_session, &stat);
  CheckStatus(sts, "- GetEncodeStat", __FILE__, __LINE__);
  stat.reserved[0] = m_Ctrl.QP;
  return stat;
}

mfxHDL CVRmfxFramework::dequeueInputBuffer() {
  if (m_unIIterator < m_unOIterator)
    CheckStatus(MFX_ERR_UNKNOWN, "- IO status error", __FILE__, __LINE__);
  // full
  if (m_unIIterator - m_unOIterator >= m_InputSurfaces.size()) return nullptr;
  if (m_bInputLocked) return nullptr;
  mfxHDLPair texpair;
  mfxStatus sts = m_allocator->GetHDL(
      m_allocator->pthis,
      m_InputSurfaces[m_unIIterator % m_InputSurfaces.size()].Data.MemId,
      &texpair.first);
  CheckStatus(sts, "- Error in GetHDL", __FILE__, __LINE__);
  m_bInputLocked = true;
  return texpair.first;
}

mfxU16 CVRmfxFramework::adjustQuality(const mfxU16 &unLastQp,
                                      const mfxU32 &unLen,
                                      const mfxU32 &unMaxLen) {
  if (unLen >= unMaxLen) {
    if (unLastQp < 51) return unLastQp + 1;
  } else {
    if (unLastQp != 0) return unLastQp - 1;
  }
  return unLastQp;
}

bool CVRmfxFramework::QueueInputBuffer() {
  if (m_unIIterator < m_unOIterator)
    CheckStatus(MFX_ERR_UNKNOWN, "- IO status error", __FILE__, __LINE__);
  // full
  if (m_unIIterator - m_unOIterator >= m_InputSurfaces.size()) return false;
  if (!m_bInputLocked) return false;
  m_unIIterator++;
  m_bInputLocked = false;
  return true;
}

bool CVRmfxFramework::Run() {
  if (m_unOIterator > m_unIIterator)
    CheckStatus(MFX_ERR_UNKNOWN, "- IO status error", __FILE__, __LINE__);
  // empty
  if (m_unOIterator == m_unIIterator) return false;
  auto bufferDepth = m_InputSurfaces.size();
  // prepare IO
  mfxFrameSurface1 *in = &m_InputSurfaces[m_unOIterator % bufferDepth];
  std::memset(&m_Output, 0, sizeof m_Output);
  m_Output.Data = m_Pool->Alloc<mfxU8 *>(m_BsBufSize);
  m_Output.MaxLength = m_BsBufSize;
  if (m_Output.Data == nullptr) return false;
  // core run
  mfxStatus sts = m_Core->RunEnc(in, &m_Output, &m_Ctrl);
  return sts == MFX_ERR_NONE;
}

int CVRmfxFramework::DequeueOutputBuffer(mfxU8 **pointer, mfxU32 *size) {
  mfxStatus sts = m_Core->MemorySync(UINT_MAX);
  *pointer = m_Output.Data + m_Output.DataOffset;
  *size = m_Output.DataLength;
  m_Output.DataOffset += m_Output.DataLength;
  m_Output.DataLength = 0;
  if (sts == MFX_ERR_NONE) {
    m_unOIterator++;
    if (m_Par.rateControl > MFX_RATECONTROL_USERDEFINED) {
      mfxF32 fMax = m_Par.targetKbps * 128.0f / m_Par.fps;
      mfxU32 uMax = static_cast<mfxU32>(ceilf(fMax));
      m_Ctrl.QP = adjustQuality(m_Ctrl.QP, m_Output.DataOffset, uMax);
    }
  }
  return sts;
}

mfxBitstream CVRmfxFramework::DequeueOutputBuffer() {
  if (m_unOIterator > m_unIIterator)
    CheckStatus(MFX_ERR_UNKNOWN, "- IO status error", __FILE__, __LINE__);
  // empty
  if (m_unOIterator == m_unIIterator) return mfxBitstream{};
  auto bufferDepth = m_InputSurfaces.size();
  // prepare IO
  mfxFrameSurface1 *in = &m_InputSurfaces[m_unOIterator % bufferDepth];
  mfxBitstream out{};
  out.Data = m_Pool->Alloc<mfxU8 *>(m_BsBufSize);
  out.MaxLength = m_BsBufSize;
  if (out.Data == nullptr) return out;
  // core run
  m_Core->RunEnc(in, &out, &m_Ctrl);
  m_Core->MemorySync(UINT_MAX);
  m_unOIterator++;
  if (m_Par.rateControl > MFX_RATECONTROL_USERDEFINED) {
    mfxF32 fMax = m_Par.targetKbps * 128.0f / m_Par.fps;
    mfxU32 uMax = static_cast<mfxU32>(ceilf(fMax));
    m_Ctrl.QP = adjustQuality(m_Ctrl.QP, out.DataLength, uMax);
  }
  return out;
}

void CVRmfxFramework::ReleaseOutputBuffer(const mfxBitstream &buf) {
  m_Pool->Dealloc(buf.Data);
}

void CVRmfxFramework::createAllocator(mfxHDL hdl) {
  mfxStatus sts = MFX_ERR_NONE;
  if (!hdl) {
    // Use system memory
    m_bSystemMemory = true;
    return;
  }
#if _WIN32
  // Use video memory
  const mfxHandleType hdl_t = MFX_HANDLE_D3D11_DEVICE;
  if (m_session.SetHandle(hdl_t, hdl) != MFX_ERR_NONE) {
    // Set multiple thread protected for MFX
    ID3D11Device *dxhdl = reinterpret_cast<ID3D11Device *>(hdl);
    ID3D10Multithread *pmt = nullptr;
    dxhdl->QueryInterface(&pmt);
    if (pmt) pmt->SetMultithreadProtected(true);
    sts = m_session.SetHandle(hdl_t, hdl);
  }
  CheckStatus(sts, "- Error in SetHandle", __FILE__, __LINE__);
  sts = m_session.SetFrameAllocator(m_allocator.get());
  CheckStatus(sts, "- Error in SetFrameAllocator", __FILE__, __LINE__);
#endif
}

}  // namespace enc
}  // namespace mfxvr
