/********************************************************************
Copyright 2017 Tang, Wenyi. All Rights Reserved.
Description
  A multi-layer encoder based on Intel MediaSDK
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 25th, 2017
Mod      :    Date      Author

********************************************************************/
#ifndef LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_MULTILAYER_H_
#define LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_MULTILAYER_H_
#include <functional>
#include <future>
#include <list>
#include <memory>
#include "ll_codec/impl/msdk/encoder/mfx_framework_enc.h"

namespace mfxvr {
namespace enc {

class CVRMultiLayer : public noncopyable {
 public:
  /**
   * Init base layer and allocate input frames.
   * \param [in] par: init parameter for base layer
   */
  explicit CVRMultiLayer(const vrpar::config &par);

  ~CVRMultiLayer();

  /**
   * Add an extra encoder layer.
   * \param [in] par: init parameter for this layer
   */
  void AddLayer(const vrpar::config &par);

  void Run(std::function<void(const mfxBitstream &bs, void *param)> task);

  bool PushInput(mfxHDL buf,
                 std::function<void(mfxHDL dst, const mfxHDL &src)> cp);

 private:
  mfxU16 m_unLayers;                       ///< number of layers
  mfxU32 m_unRunCounter;                   ///< layer index
  mfxFrameAllocResponse m_Resp;            ///< layer 0 response
  std::shared_ptr<CMFXAllocator> m_Alloc;  ///< shared allocator
  std::list<CVRmfxFramework> m_Codecs;     ///< layer instance
  std::list<mfxBitstream> m_CachedStream;
};

}  // namespace enc
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_ENCODER_DETAIL_MFX_MULTILAYER_H_
