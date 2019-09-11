/********************************************************************
Copyright 2016-2018 Tang, Wenyi. All Rights Reserved.

Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 27th, 2017
Mod      :    Date      Author

Using MediaSDK in Android as the codec
********************************************************************/
#ifndef LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H_
#define LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H_
#include <deque>
#include <map>
#include <memory>
#include <vector>
#include "ll_codec/impl/thread_safe_stl/queue/thread_safe_queue.h"
#include "ll_codec/impl/msdk/utility/mfx_alloc_base.h"
#include "ll_codec/impl/msdk/utility/mfx_base.h"
#include "ll_codec/impl/msdk/vpp/mfx_vpp_chain.h"


namespace mfxvr {
namespace dec {
class CMVCExt : public CExtBuffer<mfxExtMVCSeqDesc> {
 public:
  CMVCExt() {
    extbuff.Header.BufferId = MFX_EXTBUFF_MVC_SEQ_DESC;
    extbuff.Header.BufferSz = sizeof(extbuff);
  }

  virtual ~CMVCExt() {
    delete[] extbuff.View;
    delete[] extbuff.ViewId;
    delete[] extbuff.OP;
  }

  void AllocMVCBuffer() {
    extbuff.View = new mfxMVCViewDependency[extbuff.NumView]{};
    extbuff.ViewId = new mfxU16[extbuff.NumViewId]{};
    extbuff.OP = new mfxMVCOperationPoint[extbuff.NumOP]{};
    extbuff.NumViewAlloc = extbuff.NumView;
    extbuff.NumViewIdAlloc = extbuff.NumViewId;
    extbuff.NumOPAlloc = extbuff.NumOP;
  }
};

struct SurfaceStatus {
  mfxFrameSurface1 *surf = nullptr;
  mfxSyncPoint sync = nullptr;
  bool inuse = false;
};

/**
 * \brief The basic class for mfxSession
 *
 * Regard DecBase as a mfxSession. This class handles
 * allocation and deallocation of mfx decoder and its
 * surfaces, which are allocated by the mfx_alloc_d3d
 * or mfx_alloc_sys.
 */
class CVRDecBase : public noncopyable {
 public:
  CVRDecBase();

  virtual ~CVRDecBase();

  /**
   * Configure the codec
   * \param [inout] par the parsed infomation also stored in par.
   * \param [in] header pointer to an IDR NALU.
   * \param [in] hsize size of the NALU.
   * \throws mfx_error
   */
  void Config(vrpar::config *par, mfxU8 *header, mfxU32 hsize);

  /**
   * Check if the internal worker surfaces are free to use.
   *
   * \return false if no free surface to use.
   */
  bool InputAvaiable();

  /**
   * @brief Queue-in data w/o memcpy
   * 
   * @param src the pointer to data
   * @param size the length in bytes
   * @return true if all the data has been processed by decoder,
   *         false if some bytes remained unprocessed. If there're
   *         bytes remained, you must queue-in again and keep data
   *         available.
   */
  bool QueueInput(void *src, mfxU32 size);

  /**
   * @brief Dequeue-out decoded surface
   * 
   * @param surface is a bulk of memory if memtype is CPU,
   *        or is a handle of texture otherwise.
   */
  void DequeueOutputSurface(void **surface);

  /**
   * @brief Return the surface to decoder and unlock it.
   * 
   * @param surface the surface that acquired by DequeueOutputSurface.
   */
  void ReleaseOutputSurface(void *surface);

 private:  // func
  void initSession();

  void initParameters(vrpar::config *par);

  void initAllocator(vrpar::config *par);

  void initFrames(vrpar::config *par);

  std::vector<mfxFrameSurface1 *> getFreeSurface(int num);

 private:  // var
  MFXVideoSession sess_;
  mfxVideoParam video_params_;
  mfxBitstream input_bytes_;
  mfxBitstream cached_bytes_;
  mfxU32 old_offset_;
  mfxFrameAllocResponse responce_;
  std::vector<mfxExtBuffer *> external_buff_;
  std::vector<mfxFrameSurface1> workers_;
  std::map<mfxFrameSurface1 *, SurfaceStatus> worker_status_;
  ixr::SafeQueue<mfxFrameSurface1 *> outputs_;
  std::map<mfxHDL, mfxFrameSurface1 *> release_tab_;
  mutable std::mutex release_mutex_;
  std::unique_ptr<CMVCExt> ext_mvc_;
  std::unique_ptr<CMFXAllocator> allocator_;
  std::unique_ptr<MFXVideoDECODE> mfx_dec_;
  std::unique_ptr<vpp::VppChain> vpp_;
};
}  // namespace dec
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_DECODER_MFX_DEC_BASE_H_
