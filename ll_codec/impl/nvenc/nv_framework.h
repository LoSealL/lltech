/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description : NV encoder framework
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 11th, 2017
changelog
********************************************************************/
#ifndef LL_CODEC_NVENC_NV_FRAMEWORK_H_
#define LL_CODEC_NVENC_NV_FRAMEWORK_H_
#include <stdint.h>
#include <map>
#include <vector>
#include "ll_codec/impl/nvenc/api/nvEncodeAPI++.h"
#include "ll_codec/impl/nvenc/nv_configure.h"
#include "ll_codec/impl/nvenc/nv_error.h"


namespace nvenc {

struct NV_ENC_BITSTREAM;

/**
 * Framework for NVENC.
 */
class CVRNvFramework {
 public:
  NV_ENC_API CVRNvFramework();

  NV_ENC_API ~CVRNvFramework();

  /**
   * Initialize encoder and allocate I/O memories.
   * Must call Deallocate to finalize resources.
   */
  void NV_ENC_API Allocate(const EncodeConfig &par);

  /**
   * Deallocate must always pair to Allocate.
   * It's safe to call Deallocate multiple times.
   */
  void NV_ENC_API Deallocate();

  /**
   * Get statistics of the encoder
   */
  NV_ENC_STAT NV_ENC_API GetEncodeStatus();

  /**
   * Dequeue one buffer availiable for usage.
   */
  void *NV_ENC_API DequeueInputBuffer();

  /**
   * The NVENC use shared textures for input surfaces.
   * When input textures is ready for encoding, call QueueInputBuffer with the
   * specified texture to tell encoder to start process on this texture. The
   * encoder will signal an event on finishing. \see DequeueOutputBuffer for
   * more infomation.
   *
   * \param tex  registered texture handle.
   */
  bool NV_ENC_API QueueInputBuffer(const HANDLE &tex);

  /**
   * Dequeue output bitstream from internal memory.
   * This call will wait until the encoder outputs one frame, so don't call this
   * function in main thread. Must call ReleaseOutputBuffer to return the memory
   * back to the encoder.
   *
   * \param [out] ptr   data pointer to the bit stream.
   * \param [out] size  data size.
   */
  bool NV_ENC_API DequeueOutputBuffer(void **ptr, uint32_t *size);

  /**
   * Unlock the memory for furthur encoding.
   * \param ptr is the pointer acquired from DequeueOutputBuffer, which
   *            is not available after releasement, any access to it is
   * undefined.
   */
  void NV_ENC_API ReleaseOutputBuffer(void *ptr);

  void NV_ENC_API GetFlowControlParam(float *fps, uint32_t *throughput) const {
    *fps = static_cast<float>(m_Par.fps);
    *throughput = m_Par.bitrate;
  }

  void NV_ENC_API SetFlowControlParam(const float &fps,
                                      const uint32_t &throughput) {
    m_Par.fps = static_cast<int>(fps);
    m_Par.bitrate = throughput;
  }

 private:  // param
  using NV_EXTERN_BUF = std::map<void *, NV_ENC_REGISTERED_PTR>;
  using NV_VID_CACHE = std::map<void *, NV_ENC_BITSTREAM *>;
  std::unique_ptr<CNvEncoder> m_pCore;
  NV_ENC_CONFIG m_EncodeConfig;
  NV_ENC_INITIALIZE_PARAMS m_EncodeInitPar;
  NV_EXTERN_BUF m_CachedRegisteredResources;
  NV_VID_CACHE m_CachedVideoMemory;
  std::vector<NV_ENC_BITSTREAM> m_OutputBuffers;
  int m_nOutputRIndex;
  int m_nOutputWIndex;
  GUID m_EncodeGuid;
  EncodeConfig m_Par;

 private:  // func
  GUID getGuidFromFourCC(const uint32_t fourcc);

  void initParameters(const EncodeConfig &par);

  void allocateIObuffers(const EncodeConfig &par);

  void destroyIObuffers();

  int dequeueOutputIndex();
};
}  // namespace nvenc

#endif  // LL_CODEC_NVENC_NV_FRAMEWORK_H_
