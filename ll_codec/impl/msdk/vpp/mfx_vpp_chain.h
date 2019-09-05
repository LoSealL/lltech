/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  A series video post process method
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 25th, 2017
Mod      :    Date      Author

Updated Init and Run process. 2017.3.30
********************************************************************/
#ifndef LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
#define LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
#include <map>
#include <memory>
#include <vector>
#include "ll_codec/impl/msdk/utility/mfx_base.h"

namespace mfxvr {
namespace vpp {
/**
 * A simple VPP struct contains wrapper class
 * and relevant parameters and options
 * note that actually N vpp only have N-1 group of internal surfaces
 */
struct ultravpp {
  std::unique_ptr<MFXVideoVPP> vpp;    //!< MFX VPP class wrapper
  std::vector<mfxFrameSurface1> surf;  //!< internal frame surfaces for VPP
  std::vector<mfxExtBuffer *> ebuf;    //!< external buffer list
  std::vector<mfxSyncPoint> sync;      //!< a set of sync points
  mfxVideoParam par;                   //!< MFX VPP parameters list
  mfxFrameAllocResponse resp;  //!< responses for each allocated vpp frames
};

/**
 * A simple wrapper for Vpp.
 * This class can manage a set of vpp cores to work together.
 * Implements functions (use either individual or combined):
 *    - Resize
 *    - Crop
 *    - Mirror
 *    - MVC (for encoder)
 *    - Color space convert
 * Supported formats: RGB4, NV12
 */
class VppChain : public noncopyable {
 public:
  /**
   * Make a new instance according to i/o request.
   */
  VppChain();

  virtual ~VppChain();

  /**
   * Configure VPP parameters and allocate I/O memories.
   * This may lead to multiple vpp cores to be inited.
   *
   * \param [in] s: session that vpp will be inited from.
   * \param [in] allocator: the external memory allocator. Must be defined.
   * \param [in] par: param for i/o surface
   */
  void Alloc(mfxSession s, mfxFrameAllocator *allocator,
             const vrpar::config &par);

  /**
   * Invoke RunFrameVPPAsync for every vpp in chains one by one
   * \param [in] in: start of the input surface array
   * \param [in] out: end of the output surface array,
   *                  default we use internal surface.
   * \deprecated
   */
  mfxStatus RunVpp(mfxFrameSurface1 *in, mfxFrameSurface1 *out = nullptr);

  /***/
  mfxStatus RunVpp1(mfxFrameSurface1 *inp, mfxFrameSurface1 **outp,
                    mfxSyncPoint *sync);

  /**
   * Sync vpp operation. It's optional in encoder, but is required in decoder.
   * \param [in] wait: timeout in milisecond.
   */
  mfxStatus SyncVpp(mfxU32 wait);

  mfxStatus SyncVpp1(mfxSyncPoint sync, mfxU32 timeout);

  mfxStatus ReleaseSurface(mfxFrameSurface1 *used);

  /**
   * \return numbers of allocated vpp instance. 0 if no vpp allocated,
   * hence each API will return MFX_ERR_NOT_INITIALIZED.
   */
  mfxU32 VppChainSize() const;

  mfxStatus QueryInfo(mfxFrameInfo *info);

 protected:  // func
  /* Make default parameters */
  mfxVideoParam makeDefPar(const vrpar::surface &in,
                           const vrpar::surface &out) const;

  /* Add a new vpp core instance */
  ultravpp *pushNewOne(const vrpar::surface &in, const vrpar::surface &out);

  /* push and update extbuff to the last vpp core */
  void addExtBufLast(mfxExtBuffer *eb);

  /* allocate internal frames */
  virtual void allocFrames(const mfxFrameAllocator *alloc);

  /* invoke one vpp run */
  virtual mfxStatus runVppInternal(ultravpp *ins, mfxFrameSurface1 *in,
                                   mfxFrameSurface1 *out);

  mfxFrameSurface1 *getFreeSurface(ultravpp *ins);

 protected:                     // var
  mfxSession m_session;         //!< make a copy of session
  vrpar::config m_codec_param;  //!< make a copy of init parameters
  std::vector<ultravpp> m_vpp_list;
  std::unique_ptr<CVPPDoNotUse> m_ext_donotuse;
  std::unique_ptr<CVPPMVC> m_ext_mvc;
  std::unique_ptr<CVPPScaling> m_ext_scaling;
  std::unique_ptr<CVPPRotate> m_ext_rotate;
  std::unique_ptr<CVPPMirror> m_ext_mirror;
  std::map<mfxFrameSurface1 *, bool> m_surface_inuse;
  mfxU16 m_meta_buffer_num;
  mfxU16 m_process_id;
  bool m_use_sys_mem;
};

}  // namespace vpp
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_VPP_MFX_VPP_CHAIN_H_
