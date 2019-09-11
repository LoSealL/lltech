/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.

Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

Intel MediaSDK utilities

2017.3.24, Export VPP and ENC implementations. By Wenyi Tang.
********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_BASE_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_BASE_H_

#include <mfxdefs.h>
#include <mfxjpeg.h>
#include <mfxmvc.h>
#include <mfxplugin++.h>
#include <mfxstructures.h>
#include <mfxvideo++.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

#include "ll_codec/impl/msdk/utility/mfx_alloc_base.h"
#include "ll_codec/impl/msdk/utility/mfx_error.h"
#include "ll_codec/impl/msdk/utility/mfx_external_buffers.h"

#define MAKE_ALIGN16(x) ((x + 0x0F) >> 4 << 4)
#define MAKE_ALIGN32(x) ((x + 0x1F) >> 5 << 5)

namespace mfxvr {

inline mfxU16 getChromaFormatFromFourCC(mfxU32 fourcc) {
  switch (fourcc) {
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_BGR4:
      return MFX_CHROMAFORMAT_YUV444;
    case MFX_FOURCC_YUY2:
      return MFX_CHROMAFORMAT_YUV422V;
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_YV12:
      return MFX_CHROMAFORMAT_YUV420;
    default:
      return 0;
  }
}

enum { MFX_RATECONTROL_USERDEFINED = 100, MFX_RATECONTROL_AUTO };

enum { MFX_MEMTYPE_VR_SPECIAL = 128 };

namespace vrpar {

enum Rotate {
  ROTATE_HOLD = 0,
  ROTATE_RQUAT = 90,
  ROTATE_UDOWN = 180,
  ROTATE_LQUAT = 270
};

enum Mirror { MIRROR_NONE = 0, MIRROR_HORIZONTAL = 1, MIRROR_VERTICAL = 2 };

/* parameters for a single surface */
struct surface {
  mfxU32 color_format;  //!< FOURCC style colorspace. I.E. 'R' 'G' 'B' 'A'.
  mfxU16 width;         //!< surface width.
  mfxU16 height;        //!< surface height.
  mfxU16 cropX, cropY, cropW, cropH;  //!< crop rect
};

/* parameters for encoder/decoder */
struct config {
  mfxU32 codec;  //!< FOURCC style codec name. I.E. 'A' 'V' 'C' ' '
#ifdef _MSC_VER
  union {
    /// JPEG options
    struct {
      uint32_t jpegQuality;  //!< For JPEG encode only. Valid from 1 to 100.
      uint32_t reserved;
    };
    /// AVC options
    struct {
      uint32_t targetKbps;  //!< For AVC only.
      uint16_t rateControl;
      uint16_t gop;
    };
  };
#else
  /// Can't use anonymous struct without MSVC
  struct JpgOption {
    uint32_t jpegQuality;
    uint32_t reserved;
  } jpegOpt;
  struct AvcOption {
    uint32_t targetKbps;
    uint16_t rateControl;
    uint16_t gop;
  } avcOpt;
#endif
  surface in;   //!< Input surface
  surface out;  //!< Output surface
  mfxF32 fps;
  mfxU16 multiViewCodec;  //!< For AVC encode only. Use as boolean.
  mfxU8 mirror;  //!< Experimental, mirror the output images. As enum #Mirror
  mfxU8 rotate;  //!< Experimental, rotate the output images. As enum #Rotate
  mfxU8 enableQSVFF;  //!< enable QSV to hard-encode AVC frame
  mfxI32 asyncDepth;
  mfxI32 outputSizeMax;
  mfxI32 constQP[3];
  mfxU16 numRoi;         //!< number of regions in ROI.
  mfxI16 listRoiQPI[8];  //!< enable encoder ROI feature, the value should be
                         //!< QPI/QPP
  mfxU16 intraRefresh;   //!< Enable Error Recovery with intra refresh
  mfxU16 slice;          //!< turn on slice based encode
  mfxI32 sliceMode;
  mfxI32 sliceData;
  mfxHDL renderer;  //!< The native handle for render device.
                    //!< (ID3D11Device*/vaDisplay)
};

}  // namespace vrpar

/* Copied from boost */
class noncopyable {
 protected:
  noncopyable() = default;
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
};

/**
 * MFX session base
 */
class CVRmfxSession : public noncopyable {
 public:
  CVRmfxSession() = default;

  virtual ~CVRmfxSession() { m_allocator.reset(); }

  /**
   * Check current libmfxhw32.dll version
   * \param [out] ver: version to query
   */
  mfxStatus CheckVersion(mfxVersion *ver) {
    mfxVersion cur;
    mfxStatus sts = MFXQueryVersion(m_session, &cur);
    if (!sts && cur.Version < ver->Version) sts = MFX_ERR_UNSUPPORTED;
    *ver = cur;
    return sts;
  }

  /* Assgin external allocator */
  void AttachAllocator(std::shared_ptr<CMFXAllocator> alloc) {
    m_allocator = alloc;
  }

  mfxSession GetSession() { return static_cast<mfxSession>(m_session); }

  mfxStatus Join(mfxSession s) { return MFXJoinSession(s, m_session); }

  mfxStatus DisJoinMe() { return MFXDisjoinSession(m_session); }

 protected:
  // MFX main session
  MFXVideoSession m_session;
  // MFX external allocator
  std::shared_ptr<CMFXAllocator> m_allocator;
};

inline bool operator==(const mfxFrameSurface1 &lhs,
                       const mfxFrameSurface1 &rhs) {
  return lhs.Data.MemId == rhs.Data.MemId;
}
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_BASE_H_
