/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description : Intel MediaSDK utilities
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Mar. 24th, 2017
********************************************************************/
#include "ll_codec/impl/msdk/encoder/enc_core.h"
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <thread>

namespace mfxvr {
namespace enc {

Core::Core(mfxSession s, mfxFrameAllocator *alloc, const vrpar::config &par) {
  VppChain::Alloc(s, alloc, par);
  mfxStatus sts = initEncParams(par);
  // load hevc plugin
  if (par.codec == MFX_CODEC_HEVC) {
    sts = MFXVideoUSER_Load(s, &MFX_PLUGINID_HEVCE_HW, 1);
    CheckStatus(sts, "- Error in loading plugin", __FILE__, __LINE__);
  }
  m_MfxEnc = std::make_unique<MFXVideoENCODE>(s);
  sts = m_MfxEnc->Init(&m_EncParams);
  CheckStatus(sts, "- Error in Enc::Init", __FILE__, __LINE__);
  m_Sync.resize(1);
}

Core::~Core() { MFXVideoUSER_UnLoad(m_session, &MFX_PLUGINID_HEVCE_HW); }

mfxStatus Core::RunEnc(mfxFrameSurface1 *in, mfxBitstream *out,
                       mfxEncodeCtrl *ctrl) {
  if (!in || !out) {
    CheckStatus(MFX_ERR_NULL_PTR, "Input is null!", __FILE__, __LINE__);
  }
  mfxFrameSurface1 *vpp_out;
  mfxStatus sts = RunVpp1(in, &vpp_out, &m_Sync[0]);
  CheckStatus(sts, "RunVpp1", __FILE__, __LINE__, MFX_ERR_NOT_INITIALIZED);
  if (sts == MFX_ERR_NOT_INITIALIZED) {
    vpp_out = in;
  }
  vpp_out->Info.FrameId.ViewId = (m_process_id - 1) % m_MvcViews;
  for (;;) {
    out->DataLength = 0;
    sts = m_MfxEnc->EncodeFrameAsync(ctrl, vpp_out, out, &m_Sync[0]);
    if (sts == MFX_WRN_DEVICE_BUSY)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    else
      break;
  }
  CheckStatus(sts, "- Error in EncodeFrameAsync", __FILE__, __LINE__,
              MFX_ERR_MORE_DATA);
  ReleaseSurface(vpp_out);
  return sts;
}

mfxStatus Core::MemorySync(mfxU32 wait) {
  mfxStatus sts = MFXVideoCORE_SyncOperation(m_session, m_Sync[0], wait);
  CheckStatus(sts, "- Error in Core::Sync", __FILE__, __LINE__,
              MFX_ERR_NULL_PTR);
  return sts;
}

mfxStatus Core::QueryInfo(mfxFrameInfo *info) {
  mfxStatus sts = VppChain::QueryInfo(info);
  if (sts != MFX_ERR_NONE) {
    mfxFrameAllocRequest req{};
    sts = m_MfxEnc->QueryIOSurf(&m_EncParams, &req);
    memcpy(info, &req.Info, sizeof(mfxFrameInfo));
  }
  return sts;
}

mfxStatus Core::initEncParams(const vrpar::config &par) {
  std::memset(&m_EncParams, 0, sizeof(m_EncParams));
  m_EncParams.mfx.CodecId = par.codec;
  m_EncParams.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
  m_EncParams.mfx.RateControlMethod =
      par.rateControl ? par.rateControl : MFX_RATECONTROL_CBR;
  if (par.rateControl > MFX_RATECONTROL_USERDEFINED)
    m_EncParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
  if (m_EncParams.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
    m_EncParams.mfx.QPI = static_cast<mfxU16>(par.constQP[0]);
    m_EncParams.mfx.QPP = static_cast<mfxU16>(par.constQP[1]);
    m_EncParams.mfx.QPB = static_cast<mfxU16>(par.constQP[2]);
  } else {
    if (par.targetKbps <= std::numeric_limits<mfxU16>::max()) {
      m_EncParams.mfx.TargetKbps = static_cast<mfxU16>(par.targetKbps);
      m_EncParams.mfx.MaxKbps = static_cast<mfxU16>(par.targetKbps);
      m_EncParams.mfx.BRCParamMultiplier = 1;
    } else {
      mfxF32 scale = par.targetKbps / 65535.f;
      m_EncParams.mfx.BRCParamMultiplier = static_cast<mfxU16>(ceilf(scale));
      mfxF32 bps = ceilf(par.targetKbps / ceilf(scale));
      m_EncParams.mfx.TargetKbps =
          bps > 65535.f ? 65535 : static_cast<mfxU16>(bps);
      m_EncParams.mfx.MaxKbps = m_EncParams.mfx.TargetKbps;
    }
  }
  if (par.numRoi) {
    m_EncParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    m_EncParams.mfx.QPI = 0;
    m_EncParams.mfx.QPP = 0;
  }
  m_EncParams.mfx.GopRefDist = 1;                       // no B frame
  m_EncParams.mfx.GopPicSize = par.gop ? par.gop : 30;  // default: 30
  m_EncParams.mfx.NumRefFrame = 1;
  m_EncParams.mfx.IdrInterval = 0;
  m_EncParams.mfx.NumSlice = 0;
  m_EncParams.mfx.CodecProfile = MFX_PROFILE_AVC_HIGH;
  m_EncParams.mfx.CodecLevel = MFX_LEVEL_AVC_52;
  m_EncParams.mfx.BufferSizeInKB = 4096;
  m_EncParams.mfx.FrameInfo.FrameRateExtN =
      static_cast<mfxU32>(par.fps ? par.fps : 30);
  m_EncParams.mfx.FrameInfo.FrameRateExtD = 1;
  // binary flag, 0 signals encoder to take frames in display order
  m_EncParams.mfx.EncodedOrder = 0;
  m_EncParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
  if (par.renderer) m_EncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
  // frame info parameters
  m_EncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
  m_EncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
  m_EncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
  /**
   * width must be a multiple of 16
   * height must be a multiple of 16 in case of
   * frame picture and a multiple of 32 in case of field picture
   */
  m_EncParams.mfx.FrameInfo.Width = (par.out.width + 15) >> 4 << 4;
  m_EncParams.mfx.FrameInfo.Height = (par.out.height + 15) >> 4 << 4;
  // encoder output crop should be ignored
  m_EncParams.mfx.FrameInfo.CropW = par.out.width;
  m_EncParams.mfx.FrameInfo.CropH = par.out.height;
  /**
   * we don't specify profile and level and let the encoder choose
   * those basing on parameters we must specify profile only for MVC codec
   */
  m_MvcViews = 1;
  if (par.multiViewCodec) {
    if (MFX_CODEC_JPEG == par.codec) return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    m_EncParams.mfx.CodecProfile = MFX_PROFILE_AVC_STEREO_HIGH;
    if (!m_ext_mvc) m_ext_mvc.reset(new CVPPMVC());
    m_EncExtBuf.push_back(m_ext_mvc->getAddressOf());
    m_MvcViews = 2;
  }
  if (par.numRoi) {
    if (!m_ExtRoi) m_ExtRoi.reset(new CENCROI());
    const mfxU32 ratio[4]{5, 4, 3, 1};
    m_ExtRoi->setRoiRegion(par.in.width, par.in.height, par.numRoi,
                           par.listRoiQPI, ratio, 16);
    m_EncExtBuf.push_back(m_ExtRoi->getAddressOf());
  }
  // Intra-Refresh Configuration
  if (par.intraRefresh) {
#if 0
    std::memset(&m_CodingOption, 0, sizeof(mfxExtCodingOption));
    m_CodingOption.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    m_CodingOption.Header.BufferSz = sizeof(m_CodingOption);

    std::memset(&m_CodingOption2, 0, sizeof(mfxExtCodingOption2));
    m_CodingOption2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    m_CodingOption2.Header.BufferSz = sizeof(m_CodingOption2);

    std::memset(&m_CodingOption3, 0, sizeof(mfxExtCodingOption3));
    m_CodingOption3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
    m_CodingOption3.Header.BufferSz = sizeof(m_CodingOption3);

    m_CodingOption.RecoveryPointSEI = MFX_CODINGOPTION_ON;
    m_EncExtBuf.push_back((mfxExtBuffer *)&m_CodingOption);

    m_CodingOption2.IntRefType = 1;  // column row refresh.
    m_CodingOption2.IntRefCycleSize = 10;
    m_CodingOption2.IntRefQPDelta = 3;
    m_EncExtBuf.push_back((mfxExtBuffer *)&m_CodingOption2);

    m_CodingOption3.IntRefCycleDist = 0;
    m_EncExtBuf.push_back((mfxExtBuffer *)&m_CodingOption3);
#endif
  }
  // slice based encode
  if (par.slice) {
#if 0
    m_EncParams.mfx.NumSlice = 4;
    std::memset(&m_CodingOption3, 0, sizeof(mfxExtCodingOption3));
    m_CodingOption3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
    m_CodingOption3.Header.BufferSz = sizeof(m_CodingOption3);
    int PartialOutputMode = par.sliceMode;
    switch (PartialOutputMode - 1) {
      case MFX_PARTIAL_BITSTREAM_SLICE:
        m_CodingOption3.EnablePartialBitstreamOutput = MFX_CODINGOPTION_ON;
        m_CodingOption3.PartialBitstreamGranularity =
            MFX_PARTIAL_BITSTREAM_SLICE;
        m_CodingOption3.PartialBitstreamBlockSize =
            static_cast<mfxU16>(par.sliceData);
        // printf("partialoutput slice\r\n");
        break;
      case MFX_PARTIAL_BITSTREAM_TILE:
        m_CodingOption3.EnablePartialBitstreamOutput = MFX_CODINGOPTION_ON;
        m_CodingOption3.PartialBitstreamGranularity =
            MFX_PARTIAL_BITSTREAM_TILE;
        m_CodingOption3.PartialBitstreamBlockSize =
            static_cast<mfxU16>(par.sliceData);
        // printf("partialoutput Tile\r\n");
        break;
      case MFX_PARTIAL_BITSTREAM_BLOCK:
        m_CodingOption3.EnablePartialBitstreamOutput = MFX_CODINGOPTION_ON;
        m_CodingOption3.PartialBitstreamGranularity =
            MFX_PARTIAL_BITSTREAM_BLOCK;
        m_CodingOption3.PartialBitstreamBlockSize =
            static_cast<mfxU16>(par.sliceData);
        // printf("partialoutput block\r\n");
        break;
      case MFX_PARTIAL_BITSTREAM_ANY:
        // printf("partialoutput any\r\n");
        m_CodingOption3.EnablePartialBitstreamOutput = MFX_CODINGOPTION_ON;
        m_CodingOption3.PartialBitstreamGranularity = MFX_PARTIAL_BITSTREAM_ANY;
        m_CodingOption3.PartialBitstreamBlockSize =
            static_cast<mfxU16>(par.sliceData);
        break;
      default:
        // printf("partialoutput off\r\n");
        m_CodingOption3.EnablePartialBitstreamOutput = MFX_CODINGOPTION_OFF;
        break;
    }
    m_EncExtBuf.push_back((mfxExtBuffer *)&m_CodingOption3);
#endif
  }
  if (!m_EncExtBuf.empty()) {
    m_EncParams.ExtParam = &m_EncExtBuf[0];
    m_EncParams.NumExtParam = static_cast<mfxU16>(m_EncExtBuf.size());
  }
  // JPEG encoder settings overlap with other encoders
  // settings in mfxInfoMFX structure
  if (MFX_CODEC_JPEG == par.codec) {
    m_EncParams.mfx.Interleaved = 1;
    m_EncParams.mfx.Quality = static_cast<mfxU16>(par.jpegQuality);
    m_EncParams.mfx.RestartInterval = 0;
    m_EncParams.mfx.CodecProfile = 0;
    m_EncParams.mfx.CodecLevel = 0;
    std::memset(m_EncParams.mfx.reserved5, 0, sizeof(m_EncParams.mfx.reserved5));
  }
  // Enable AVC encoding with Fixed Function, which benifit the latency and
  // offloading 3D workload.
  if (par.enableQSVFF) {
    m_EncParams.mfx.LowPower = MFX_CODINGOPTION_ON;
  }
  m_EncParams.AsyncDepth = 1;
  return MFX_ERR_NONE;
}
}  // namespace enc
}  // namespace mfxvr
