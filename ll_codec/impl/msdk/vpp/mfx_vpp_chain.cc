/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description
  A series video post process method
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 25th, 2017
Mod      :    Date      Author

Updated Init and Run process. 2017.3.30
********************************************************************/
#include "ll_codec/impl/msdk/vpp/mfx_vpp_chain.h"
#include <algorithm>
#include <thread>

namespace mfxvr {
namespace vpp {
VppChain::VppChain() : m_process_id(0) {}

VppChain::~VppChain() {}

void VppChain::Alloc(mfxSession s, mfxFrameAllocator *allocator,
                     const vrpar::config &par) {
  m_session = s;
  m_codec_param = par;
  m_meta_buffer_num = static_cast<mfxU16>(par.asyncDepth);
  mfxIMPL impl;
  MFXQueryIMPL(s, &impl);
  if ((m_codec_param.in.color_format != m_codec_param.out.color_format) ||
      (m_codec_param.in.width != m_codec_param.out.width) ||
      (m_codec_param.in.height != m_codec_param.out.height)) {
    pushNewOne(m_codec_param.in, m_codec_param.out);
    // MVC
    if (m_codec_param.multiViewCodec > 0) {
      m_ext_mvc.reset(new CVPPMVC());
      addExtBufLast(m_ext_mvc->getAddressOf());
    }
    if (!(impl & MFX_IMPL_SOFTWARE)) {
      // use scaling for csc
      m_ext_scaling.reset(new CVPPScaling());
      addExtBufLast(m_ext_scaling->getAddressOf());
    }
  }
  // Scaling, Rotate and Mirror are not compatible
  if (m_codec_param.rotate && m_codec_param.renderer) {
    pushNewOne(m_codec_param.out, m_codec_param.out);
    m_ext_rotate.reset(new CVPPRotate());
    addExtBufLast(m_ext_rotate->getAddressOf());
  }
  if (m_codec_param.mirror && m_codec_param.renderer) {
    pushNewOne(m_codec_param.out, m_codec_param.out);
    m_ext_mirror.reset(new CVPPMirror());
    addExtBufLast(m_ext_mirror->getAddressOf());
  }
  m_use_sys_mem = m_codec_param.renderer == nullptr;
  // allocate vpp intermediate and output surfaces
  allocFrames(allocator);
  for (auto &&vc : m_vpp_list) {
    mfxStatus sts = vc.vpp->Query(&vc.par, &vc.par);
    sts = vc.vpp->Init(&vc.par);
    CheckStatus(sts, "- Error in VPP::Init", __FILE__, __LINE__);
  }
}

mfxStatus VppChain::RunVpp(mfxFrameSurface1 *in, mfxFrameSurface1 *out) {
  mfxStatus sts = MFX_ERR_NOT_INITIALIZED;
  // if no vpp
  if (m_vpp_list.empty()) return sts;
  auto vpp_cur = m_vpp_list.begin();
  auto vpp_pre = m_vpp_list.begin();
  auto vpp_last = m_vpp_list.end();
  if (!out) {
    out = m_vpp_list.back().surf.data();
  }
  m_process_id++;
  if (vpp_cur == --vpp_last) {
    // if only have one vpp
    return runVppInternal(&*vpp_cur, in, out);
  } else {
    // run the 1st vpp
    sts = runVppInternal(&*vpp_cur, in, vpp_cur->surf.data());
  }
  // loop to have all vpp works done
  for (;;) {
    vpp_pre = vpp_cur++;
    // don't run last vpp
    if (vpp_cur == vpp_last) break;
    // run internal vpp
    sts = runVppInternal(&*vpp_cur, vpp_pre->surf.data(), vpp_cur->surf.data());
  }
  // run last vpp
  sts = runVppInternal(&*vpp_last, vpp_pre->surf.data(), out);
  return sts;
}

mfxStatus VppChain::SyncVpp(mfxU32 wait) {
  mfxStatus sts = MFX_ERR_NONE;
  if (m_vpp_list.empty()) return sts;
  for (mfxU16 i = 0; i < m_meta_buffer_num; ++i) {
    sts =
        MFXVideoCORE_SyncOperation(m_session, m_vpp_list.back().sync[i], wait);
    CheckStatus(sts, "Vpp SyncOperation", __FILE__, __LINE__, MFX_ERR_NULL_PTR);
  }
  return sts;
}

mfxStatus VppChain::RunVpp1(mfxFrameSurface1 *inp, mfxFrameSurface1 **outp,
                            mfxSyncPoint *sync) {
  // if no vpp
  if (m_vpp_list.empty()) return MFX_ERR_NOT_INITIALIZED;
  m_process_id++;
  mfxFrameSurface1 *vpp_in = inp, *vpp_out = nullptr;
  for (auto &ins : m_vpp_list) {
    vpp_out = getFreeSurface(&ins);
    if (!vpp_out) return MFX_ERR_MORE_SURFACE;
    mfxStatus sts = runVppInternal(&ins, vpp_in, vpp_out);
    CheckStatus(sts, "RunVppAsync", __FILE__, __LINE__);
    vpp_in = vpp_out;
    *outp = vpp_out;
    *sync = ins.sync[0];
  }
  m_surface_inuse[*outp] = true;
  return MFX_ERR_NONE;
}

mfxStatus VppChain::SyncVpp1(mfxSyncPoint sync, mfxU32 timeout) {
  if (m_vpp_list.empty()) return MFX_ERR_NOT_INITIALIZED;
  mfxStatus sts = MFXVideoCORE_SyncOperation(m_session, sync, timeout);
  CheckStatus(sts, "Vpp SyncOperation", __FILE__, __LINE__, MFX_ERR_NULL_PTR);
  return sts;
}

mfxStatus VppChain::ReleaseSurface(mfxFrameSurface1 *used) {
  if (m_vpp_list.empty()) return MFX_ERR_NOT_INITIALIZED;
  if (m_surface_inuse[used]) {
    m_surface_inuse[used] = false;
  } else {
    return MFX_ERR_NOT_FOUND;
  }
  return MFX_ERR_NONE;
}

mfxU32 VppChain::VppChainSize() const { return mfxU32(m_vpp_list.size()); }

mfxVideoParam VppChain::makeDefPar(const vrpar::surface &in,
                                   const vrpar::surface &out) const {
  mfxVideoParam pardefault{};
  pardefault.IOPattern =
      MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
  if (m_codec_param.renderer) {
    pardefault.IOPattern =
        MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
  }
  pardefault.vpp.In.FourCC = in.color_format;
  pardefault.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
  pardefault.vpp.In.ChromaFormat = getChromaFormatFromFourCC(in.color_format);
  pardefault.vpp.In.Width = (in.width + 15) >> 4 << 4;
  pardefault.vpp.In.Height = (in.height + 15) >> 4 << 4;
  pardefault.vpp.In.CropW = in.cropW;
  pardefault.vpp.In.CropH = in.cropH;
  pardefault.vpp.In.CropX = in.cropX;
  pardefault.vpp.In.CropY = in.cropY;
  pardefault.vpp.In.FrameRateExtN = 30;
  pardefault.vpp.In.FrameRateExtD = 1;

  pardefault.vpp.Out.FourCC = out.color_format;
  pardefault.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
  pardefault.vpp.Out.ChromaFormat = getChromaFormatFromFourCC(out.color_format);
  pardefault.vpp.Out.Width = (out.width + 15) >> 4 << 4;
  pardefault.vpp.Out.Height = (out.height + 15) >> 4 << 4;
  pardefault.vpp.Out.CropW = out.cropW;
  pardefault.vpp.Out.CropH = out.cropH;
  pardefault.vpp.Out.CropX = out.cropX;
  pardefault.vpp.Out.CropY = out.cropY;
  pardefault.vpp.Out.FrameRateExtN = 30;
  pardefault.vpp.Out.FrameRateExtD = 1;
  return pardefault;
}

ultravpp *VppChain::pushNewOne(const vrpar::surface &in,
                               const vrpar::surface &out) {
  m_vpp_list.push_back(ultravpp());
  m_vpp_list.back().vpp = std::make_unique<MFXVideoVPP>(m_session);
  m_vpp_list.back().par = makeDefPar(in, out);
  // VPP DoNotUse
  m_ext_donotuse.reset(new CVPPDoNotUse());
  addExtBufLast(m_ext_donotuse->getAddressOf());
  m_vpp_list.back().par.AsyncDepth = m_meta_buffer_num;
  return &m_vpp_list.back();
}

void VppChain::addExtBufLast(mfxExtBuffer *eb) {
  m_vpp_list.back().ebuf.push_back(eb);
  m_vpp_list.back().par.ExtParam = &m_vpp_list.back().ebuf[0];
  m_vpp_list.back().par.NumExtParam++;
}

void VppChain::allocFrames(const mfxFrameAllocator *alloc) {
  // if no vpp needed
  if (m_vpp_list.empty()) return;
  if (!alloc)
    CheckStatus(MFX_ERR_NULL_PTR, "Allocator is NULL", __FILE__, __LINE__);
  // vpp_req[0] is for input request,
  // vpp_req[1] is for output request
  mfxFrameAllocRequest vpp_req[2]{};
  for (auto &&vpp : m_vpp_list) {
    mfxStatus sts = vpp.vpp->QueryIOSurf(&vpp.par, vpp_req);
    CheckStatus(sts, "- Error in VPP::QueryIOSurf", __FILE__, __LINE__);
    vpp_req[1].NumFrameMin = vpp_req[1].NumFrameSuggested;
    vpp_req[1].Info = vpp.par.vpp.Out;
    vpp_req[1].Type |= MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_VR_SPECIAL;
    sts = alloc->Alloc(alloc->pthis, &vpp_req[1], &vpp.resp);
    CheckStatus(sts, "- Error in internal Vpp::Alloc", __FILE__, __LINE__);
    vpp.surf.resize(vpp.resp.NumFrameActual);
    vpp.sync.resize(1);
    int i = 0;
    for (auto &&surf : vpp.surf) {
      surf.Info = vpp.par.vpp.Out;
      if (m_use_sys_mem) {
        sts = alloc->Lock(alloc->pthis, vpp.resp.mids[i++], &surf.Data);
        CheckStatus(sts, "- Error in Alloc::Lock", __FILE__, __LINE__);
      } else {
        surf.Data.MemId = vpp.resp.mids[i++];
      }
    }
  }
}

mfxStatus VppChain::QueryInfo(mfxFrameInfo *info) {
  if (m_vpp_list.empty()) return MFX_ERR_NULL_PTR;
  mfxStatus sts;
  mfxFrameAllocRequest req[2]{};
  sts = m_vpp_list.front().vpp->QueryIOSurf(&m_vpp_list.front().par, req);
  memcpy(info, &req[0].Info, sizeof(mfxFrameInfo));
  return sts;
}

mfxStatus VppChain::runVppInternal(ultravpp *ins, mfxFrameSurface1 *in,
                                   mfxFrameSurface1 *out) {
  mfxStatus sts;
  if (!in || !out) return MFX_ERR_NULL_PTR;
  mfxU16 outputOffset = (m_process_id - 1) % m_meta_buffer_num;
  in->Info.FrameId.ViewId = outputOffset;
  for (;;) {
    sts = ins->vpp->RunFrameVPPAsync(in, out, nullptr, &ins->sync[0]);
    if (sts == MFX_WRN_DEVICE_BUSY) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      break;
    }
  }
  CheckStatus(sts, "- Error in RunFrameVPPAsync", __FILE__, __LINE__);
  return MFX_ERR_NONE;
}

mfxFrameSurface1 *VppChain::getFreeSurface(ultravpp *ins) {
  for (auto &surf : ins->surf) {
    if (surf.Data.Locked == 0 && !m_surface_inuse[&surf]) {
      return &surf;
    }
  }
  return nullptr;
}

}  // namespace vpp
}  // namespace mfxvr
