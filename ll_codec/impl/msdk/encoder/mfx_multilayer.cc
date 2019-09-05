/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  A multi-layer encoder based on Intel MediaSDK
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 25th, 2017
Mod      :    Date      Author

********************************************************************/
#include "ll_codec/impl/msdk/encoder/mfx_multilayer.h"
#include <algorithm>
#if _WIN32
#include "ll_codec/impl/msdk/utility/mfx_alloc_d3d.h"
#endif
#include "ll_codec/impl/msdk/utility/mfx_alloc_sys.h"


namespace mfxvr {
namespace enc {
CVRMultiLayer::CVRMultiLayer(const vrpar::config &par)
    : m_unLayers(1), m_unRunCounter(0) {
  mfxStatus sts;
  mfxFrameAllocRequest req{};
  if (par.renderer) {
#if _WIN32
    m_Alloc.reset(new CVRDX11Allocator(par.renderer, false));
#endif
  } else {
    m_Alloc.reset(new CVRSysAllocator());
    req.Type = MFX_MEMTYPE_SYSTEM_MEMORY;
  }
  // Alloc shared input buffers
  req.NumFrameMin = req.NumFrameSuggested = 8;
  req.Type |= MFX_MEMTYPE_FROM_VPPIN;
  req.Info.FourCC = par.in.color_format;
  req.Info.Width = par.in.width;
  req.Info.Height = par.in.height;
  sts = m_Alloc->Alloc(m_Alloc->pthis, &req, &m_Resp);
  CheckStatus(sts, "- Error in Alloc input frames", __FILE__, __LINE__);
}

CVRMultiLayer::~CVRMultiLayer() {
  for (auto &&session : m_Codecs) {
    session.DisJoinMe();
  }
}

void CVRMultiLayer::AddLayer(const vrpar::config &par) {
  m_Codecs.emplace_back();
  if (m_Codecs.size() > 1) {
    CheckStatus(m_Codecs.back().Join(m_Codecs.begin()->GetSession()),
                "- Error in Join");
  }
  // Share the same input surfaces
  m_Codecs.back().AttachAllocator(m_Alloc);
  m_Codecs.back().Allocate(par, m_Resp);
  m_unLayers++;
}

void CVRMultiLayer::Run(
    std::function<void(const mfxBitstream &bs, void *param)> task) {
  assert(!m_Codecs.empty());
  for (auto &&child : m_Codecs) {
    auto &&bs = child.DequeueOutputBuffer();
    if (bs.Data && bs.DataLength > 0) {
      task(bs, nullptr);
    }
    child.ReleaseOutputBuffer(bs);
  }
}

bool CVRMultiLayer::PushInput(
    mfxHDL buf, std::function<void(mfxHDL dst, const mfxHDL &src)> cp) {
  assert(!m_Codecs.empty());
  auto input = m_Codecs.begin()->DequeueInputBuffer(buf);
  if (!input) return false;
  cp(input, buf);
  for (auto &&child : m_Codecs) {
    child.QueueInputBuffer();
  }
  return true;
}

}  // namespace enc
}  // namespace mfxvr
