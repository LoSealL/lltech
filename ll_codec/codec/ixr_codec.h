/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 14th, 2017
********************************************************************/
#ifndef LL_CODEC_CODEC_IXR_CODEC_H_
#define LL_CODEC_CODEC_IXR_CODEC_H_
#include <memory>
#include "ll_codec/codec/ixr_codec_def.h"

namespace ixr {
/**
 * @brief IXR::Encoder interface
 *
 * Create an object by static method Encoder::Create()
 */
class IXR_CODEC_API Encoder {
 public:
  virtual ~Encoder();
  /**
   * @brief Allocate resources.
   *
   * Must be called before other functions, must call
   * Deallocate to release the resources.
   *
   * @param config specifies encoder configurations
   */
  virtual void Allocate(const CodecConfig &config);

  /**
   * @brief Release the resources and delete the object
   * Miss Deallocate could cause memory leak!
   */
  virtual void Deallocate();

  /**
   * @brief Get the Encode Status object
   *
   * @return CodecStat
   */
  virtual CodecStat GetEncodeStatus();

  /**
   * @brief Dequeue an internal input buffer.
   *
   * If CodecConfig::device and IXR_MEM_*_GPU are specified,
   * this function will return texture handle as ID3D11Texture2D*.
   * Otherwise returns a pointer to CPU memory buffer.
   *
   * @return void* pointer
   */
  virtual void *DequeueInputBuffer();

  /**
   * @brief Queue back input buffer acquired by DequeueInputBuffer()
   *
   * You should call QueueInputBuffer right after DequeueInputBuffer,
   * otherwise the buffer could be overwritten.
   *
   * @param ptr the memory handle which is acquired by DequeueInputBuffer()
   * @return 0 if succeed, -1 otherwise.
   */
  virtual int QueueInputBuffer(void *ptr);

  /**
   * @brief Queue in a user defined structure as a FIFO.
   * Data is copied into internal FIFO.
   *
   * @param data structure header
   * @param size structure size
   * @return 0 if succeed, -1 otherwise.
   */
  virtual int QueueUserData(void *data, uint32_t size);

  /**
   * @brief Dequeue out user defined structure as a FIFO.
   * Data is copied from internal FIFO
   *
   * @param data pointer to an allocated memory
   * @param size structure size
   * @return 0 if succeed, -1 otherwise.
   */
  virtual int DequeueUserData(void *data, uint32_t *size);

  /**
   * @brief Synchronize the encoding operation and dequeue the output bitstream
   *
   * This will dequeue a complete NALU or JPEG frame. The internal bitstream is
   * locked afterward. This function is thread-safe with
   * DequeueInputBuffer/QueueInputBuffer.
   *
   * @param ptr pointer to the bitstream memory
   * @param size length of the bitstream
   * @return 0 if succeed, -1 otherwise.
   */
  virtual int DequeueOutputBuffer(void **ptr, uint32_t *size);

  /**
   * @brief Unlock the internal bitstream.
   *
   * You should release every bitstream acquired by DequeueOutputBuffer
   *
   * @param ptr pointer to the bitstream.
   */
  virtual void ReleaseOutputBuffer(void *ptr);

  /**
   * @brief Get the Flow Control Parameters
   *
   * @param fps the current frame per seconds
   * @param throughput the current throughput, in kbps
   */
  virtual void GetFlowControlParam(float *fps, uint32_t *throughput) const;

  /**
   * @brief Set the Flow Control Parameters
   *
   * This will affect the output bps only with IXR_RC_MODE_AUTO.
   * @param fps specify the new FPS
   * @param throughput specify the new throughput in kbps
   */
  virtual void SetFlowControlParam(const float fps, const uint32_t throughput);

  struct ConfigInfo {
    AdapterVendor vid;
    CodecConfig *config;
  };
  /**
   * @brief Create the implementation, you can't new Encoder since it doesn't
   * have ctor.
   *
   * @param AdapterVendor @see AdapterVendor. Support INTEL and NVIDIA adapter
   * for now.
   */
  static std::unique_ptr<Encoder> Create(AdapterVendor);
  static std::shared_ptr<Encoder> Create(ConfigInfo &info);
};

/**
 * @brief IXR::Decoder interface
 *
 * Create an object by static method Decoder::Create()
 */
class IXR_CODEC_API Decoder {
 public:
  virtual ~Decoder();

  /**
   * @brief Allocate resources
   *
   * Must be called before other functions, must call
   * Deallocate to release the resources. The NALU header parsed
   * infomation will be written back to config.
   *
   * @param config specify codec configurations
   * @param nalu must be a complete IDR frame or JPEG header
   * @param size size of the NALU
   *
   * @note For now only width and height information are written to config.
   */
  virtual void Allocate(CodecConfig &config, void *nalu, uint32_t size);

  /**
   * @deprecated
   * @brief Release the allocated resources and delete object
   * You must call Deallocate pair with Allocate. Otherwise memory leaks.
   */
  virtual void Deallocate();

  /** @deprecated */
  virtual CodecStat GetDecodeStatus();

  /**
   * @brief Queue bytes into decoder.
   *
   * Bytes are not copied and must keep available until function returns.
   *
   * @param ptr pointer to the bytes.
   * @param size bytes length
   * @return 0 if succeed, -1 otherwise.
   *
   * @note This function could be blocked if no more free surfaces, hence
   * it's recommended not to call in main thread. DO NOT call in the same
   * thread with ReleaseOutputBuffer, or ReleaseOutputBuffer may never be
   * called.
   */
  virtual int QueueInputBuffer(void *ptr, uint32_t size);

  /**
   * @brief Dequeue an output buffer handle if outputs available.
   *
   * @param ptr is pointer to CPU memory if specified IXR_MEM_*_CPU,
   *            or is pointer to ID3D11Texture2D* if set IXR_MEM_*_GPU.
   * @return 0 if succeed, -1 otherwise.
   *
   * @note the output surface will be marked as locked and no more a
   * free surface. You should release it afterward.
   */
  virtual int DequeueOutputBuffer(void **ptr);

  /**
   * @brief Unlock the output surface.
   *
   * You should release every output surface dequeued by DequeueOutputBuffer,
   * otherwise QueueInputBuffer will stuck in a dead lock.
   *
   * @param ptr the output surface handle.
   */
  virtual void ReleaseOutputBuffer(void *ptr);

  /** @deprecated api */
  virtual void GetPrivateData(void *data) const;
  virtual void SetPrivateData(void *data);

  struct ConfigInfo {
    AdapterVendor vid;
    CodecConfig *config;
    void *nalu;
    uint32_t nalu_size;
  };
  /**
   * @brief Create the implementation, you can't new Decoder since it doesn't
   * have ctor.
   *
   * @param AdapterVendor @see AdapterVendor. Support only INTEL adapter for
   * now.
   */
  static std::unique_ptr<Decoder> Create(AdapterVendor);
  static std::shared_ptr<Decoder> Create(ConfigInfo &info);
};

/**
 * @brief IXR::Vpp interface
 *
 * Vpp stands for video post process.
 * Create an object by static method Vpp::Create()
 */
class IXR_CODEC_API Vpp {
 public:
  virtual ~Vpp();
  virtual void Allocate(const CodecConfig &config);
  virtual void Deallocate();
  virtual void *DequeueInputBuffer();
  virtual int QueueInputBuffer(void *ptr);
  virtual int DequeueOutputBuffer(void **ptr, uint32_t *size);
  virtual void ReleaseOutputBuffer(void *ptr);

  struct ConfigInfo {
    AdapterVendor vid;
    CodecConfig *config;
  };
  /**
   * @brief Create the implementation, you can't new Vpp since it doesn't have
   * ctor.
   *
   * @param AdapterVendor @see AdapterVendor. Support only INTEL adapter for
   * now.
   */
  static std::unique_ptr<Vpp> Create(AdapterVendor);
  static std::shared_ptr<Vpp> Create(ConfigInfo &info);
};
}  // namespace ixr
#endif  // LL_CODEC_CODEC_IXR_CODEC_H_
