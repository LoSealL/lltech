/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.

Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

Intel MediaSDK external buffer wrappers
********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_EXTERNAL_BUFFERS_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_EXTERNAL_BUFFERS_H_
#include <mfxdefs.h>
#include <mfxstructures.h>
#include "ll_codec/impl/msdk/utility/mfx_error.h"

namespace mfxvr {
/* External buffer template */
template <typename T>
class CExtBuffer {
 public:
  CExtBuffer() { std::memset(&extbuff, 0, sizeof(T)); }

  mfxExtBuffer *getAddressOf() {
    return reinterpret_cast<mfxExtBuffer *>(&extbuff);
  }

  T *get() { return &extbuff; }

 protected:
  T extbuff;
};

class CVPPDoNotUse : public CExtBuffer<mfxExtVPPDoNotUse> {
 public:
  CVPPDoNotUse() {
    extbuff.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    extbuff.Header.BufferSz = sizeof(extbuff);
    extbuff.NumAlg = 4;
    extbuff.AlgList = new mfxU32[extbuff.NumAlg];
    // turn off denoising (on by default)
    extbuff.AlgList[0] = MFX_EXTBUFF_VPP_DENOISE;
    // turn off scene analysis (on by default)
    extbuff.AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS;
    // turn off detail enhancement (on by default)
    extbuff.AlgList[2] = MFX_EXTBUFF_VPP_DETAIL;
    // turn off processing amplified (on by default)
    extbuff.AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP;
  }

  virtual ~CVPPDoNotUse() { delete[] extbuff.AlgList; }
};

class CVPPScaling : public CExtBuffer<mfxExtVPPScaling> {
 public:
  CVPPScaling() {
    extbuff.Header.BufferId = MFX_EXTBUFF_VPP_SCALING;
    extbuff.Header.BufferSz = sizeof(extbuff);
    extbuff.ScalingMode = MFX_SCALING_MODE_LOWPOWER;
  }

  virtual ~CVPPScaling() {}
};

class CVPPMirror : public CExtBuffer<mfxExtVPPMirroring> {
 public:
  CVPPMirror() {
    extbuff.Header.BufferId = MFX_EXTBUFF_VPP_MIRRORING;
    extbuff.Header.BufferSz = sizeof(extbuff);
  }

  virtual ~CVPPMirror() {}

  void SetType(mfxU16 type) { extbuff.Type = type; }
};

class CVPPRotate : public CExtBuffer<mfxExtVPPRotation> {
 public:
  CVPPRotate() {
    extbuff.Header.BufferId = MFX_EXTBUFF_VPP_ROTATION;
    extbuff.Header.BufferSz = sizeof(extbuff);
  }

  virtual ~CVPPRotate() {}

  void SetAngle(mfxU16 r) { extbuff.Angle = r; }
};

class CVPPMVC : public CExtBuffer<mfxExtMVCSeqDesc> {
 public:
  CVPPMVC() {
    extbuff.Header.BufferId = MFX_EXTBUFF_MVC_SEQ_DESC;
    extbuff.Header.BufferSz = sizeof(extbuff);

    extbuff.NumView = extbuff.NumViewAlloc = 2;
    extbuff.NumViewId = extbuff.NumViewIdAlloc = 2;
    extbuff.NumOP = extbuff.NumOPAlloc = 1;

    extbuff.View = new mfxMVCViewDependency[extbuff.NumViewAlloc];
    for (mfxU16 i = 0; extbuff.NumViewAlloc > i; ++i) {
      extbuff.View[i].ViewId = i;
    }
    extbuff.View[1].NumAnchorRefsL0 = 1;
    extbuff.View[1].AnchorRefL0[0] = 0;
    extbuff.View[1].NumNonAnchorRefsL0 = 1;
    extbuff.View[1].NonAnchorRefL0[0] = 0;

    extbuff.ViewId = new mfxU16[extbuff.NumViewIdAlloc];
    for (mfxU16 i = 0; extbuff.NumViewIdAlloc > i; ++i) {
      extbuff.ViewId[i] = i;
    }

    extbuff.OP = new mfxMVCOperationPoint[extbuff.NumOPAlloc];
    for (mfxU16 i = 0; extbuff.NumOPAlloc > i; ++i) {
      std::memset(&extbuff.OP[i], 0, sizeof(extbuff.OP[i]));
      extbuff.OP[i].NumViews = 2;
      extbuff.OP[i].NumTargetViews = 2;
      extbuff.OP[i].TargetViewId = extbuff.ViewId;
    }
  }

  virtual ~CVPPMVC() {
    delete[] extbuff.View;
    delete[] extbuff.ViewId;
    delete[] extbuff.OP;
  }
};

class CENCROI : public CExtBuffer<mfxExtEncoderROI> {
 public:
  CENCROI() {
    extbuff.Header.BufferId = MFX_EXTBUFF_ENCODER_ROI;
    extbuff.Header.BufferSz = sizeof(extbuff);
    extbuff.NumROI = 1;
    extbuff.ROIMode = MFX_ROI_MODE_QP_DELTA;
  }

  void setRoiRegion(const mfxU32 width, const mfxU32 height,
                    const mfxU32 regions, const mfxI16 *qpi,
                    const mfxU32 *ratioN, const mfxU32 ratioD = 16) {
    extbuff.NumROI = static_cast<mfxU16>(regions);
    if (regions >= 256 || !qpi || !ratioN || !ratioD) {
      throw CVRMFXError("ROI Region overflow!", MFX_ERR_INVALID_VIDEO_PARAM);
    }
    for (mfxU32 i = 0; i < regions; i++) {
      extbuff.ROI[i].Left = width * ratioN[i] / ratioD;
      extbuff.ROI[i].Right = width * (ratioD - ratioN[i]) / ratioD;
      extbuff.ROI[i].Top = height * ratioN[i] / ratioD;
      extbuff.ROI[i].Bottom = height * (ratioD - ratioN[i]) / ratioD;
      extbuff.ROI[i].DeltaQP = qpi[i];
    }
  }
};
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_EXTERNAL_BUFFERS_H_
