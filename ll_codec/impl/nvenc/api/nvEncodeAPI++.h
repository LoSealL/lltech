/********************************************************************
Copyright 2017 Intel Corp. All Rights Reserved.
Description : c++ class for nvEncoderAPI
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 11th, 2017
changelog
********************************************************************/
#ifndef LL_CODEC_NVENC_API_NVENCODEAPI$$_H_
#define LL_CODEC_NVENC_API_NVENCODEAPI$$_H_
#include "nvEncodeAPI.h"
#include <d3d11.h>
#include <memory>


namespace nvenc {
class CNvEncoder {
 public:
  CNvEncoder() : m_pNvApi(nullptr), m_hEncSession(nullptr) {}

  virtual ~CNvEncoder() { CloseEncodeSession(); }

  bool LoadLibraryAPI() {
    typedef NVENCSTATUS(NVENCAPI * MYPROC)(NV_ENCODE_API_FUNCTION_LIST *);
    HMODULE hApi = NULL;
#ifdef _WIN64
    hApi = LoadLibraryA("nvEncodeAPI64.dll");
#else
    hApi = LoadLibraryA("nvEncodeAPI.dll");
#endif
    if (!hApi) return false;
    MYPROC getApiHeader =
        (MYPROC)GetProcAddress(hApi, "NvEncodeAPICreateInstance");
    if (!getApiHeader) return false;
    m_pNvApi = std::make_unique<NV_ENCODE_API_FUNCTION_LIST>();
    m_pNvApi->version = NV_ENCODE_API_FUNCTION_LIST_VER;
    NVENCSTATUS sts = getApiHeader(m_pNvApi.get());
    if (sts != NV_ENC_SUCCESS) {
      m_pNvApi.reset();
      return false;
    }
    return true;
  }

  NV_ENCODE_API_FUNCTION_LIST *GetNativeApi() const { return m_pNvApi.get(); }

  HANDLE GetEncoderSession() const { return m_hEncSession; }

  NVENCSTATUS OpenEncodeSessionEx(const HANDLE &device) {
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS par{};
    par.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    par.apiVersion = NVENCAPI_VERSION;
    par.device = device;
    par.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
    return m_pNvApi->nvEncOpenEncodeSessionEx(&par, &m_hEncSession);
  }

  void CloseEncodeSession() {
    if (m_hEncSession) {
      m_pNvApi->nvEncDestroyEncoder(m_hEncSession);
      m_hEncSession = nullptr;
    }
  }

  bool IsSupportEncodeGUID(const GUID &guid) {
    uint32_t encodeGuidCount = 0;
    NVENCSTATUS sts;
    sts = m_pNvApi->nvEncGetEncodeGUIDCount(m_hEncSession, &encodeGuidCount);
    if (sts != NV_ENC_SUCCESS) return false;
    GUID *encodeGuidArray = new GUID[encodeGuidCount];
    uint32_t guidArraySize = 0;
    sts = m_pNvApi->nvEncGetEncodeGUIDs(m_hEncSession, encodeGuidArray,
                                        encodeGuidCount, &guidArraySize);
    if (sts != NV_ENC_SUCCESS) goto exit;
    sts = NV_ENC_ERR_GENERIC;
    for (auto i = 0U; i < guidArraySize; ++i) {
      if (encodeGuidArray[i] == guid) {
        sts = NV_ENC_SUCCESS;
        break;
      }
    }
  exit:
    delete[] encodeGuidArray;
    return sts == NV_ENC_SUCCESS;
  }

  bool IsSupportEncodeProfile(const GUID &codec, const GUID profile) {
    NVENCSTATUS sts;
    uint32_t profileCount = 0;
    sts = m_pNvApi->nvEncGetEncodeProfileGUIDCount(m_hEncSession, codec,
                                                   &profileCount);
    if (sts != NV_ENC_SUCCESS) return false;
    GUID *profileArray = new GUID[profileCount];
    uint32_t profileSize = 0;
    sts = m_pNvApi->nvEncGetEncodeProfileGUIDs(
        m_hEncSession, codec, profileArray, profileCount, &profileSize);
    if (sts != NV_ENC_SUCCESS) goto exit;
    sts = NV_ENC_ERR_GENERIC;
    for (auto i = 0U; i < profileSize; ++i) {
      if (profile == profileArray[i]) {
        sts = NV_ENC_SUCCESS;
        break;
      }
    }
  exit:
    delete[] profileArray;
    return sts == NV_ENC_SUCCESS;
  }

  bool IsSupportInputFormat(const GUID &codec,
                            const NV_ENC_BUFFER_FORMAT format) {
    NVENCSTATUS sts;
    uint32_t formatCount = 0;
    sts =
        m_pNvApi->nvEncGetInputFormatCount(m_hEncSession, codec, &formatCount);
    if (sts != NV_ENC_SUCCESS) return false;
    NV_ENC_BUFFER_FORMAT *formatArray = new NV_ENC_BUFFER_FORMAT[formatCount];
    uint32_t formatSize = 0;
    sts = m_pNvApi->nvEncGetInputFormats(m_hEncSession, codec, formatArray,
                                         formatCount, &formatSize);
    if (sts != NV_ENC_SUCCESS) goto exit;
    sts = NV_ENC_ERR_GENERIC;
    for (auto i = 0U; i < formatSize; ++i) {
      if (format == formatArray[i]) {
        sts = NV_ENC_SUCCESS;
        break;
      }
    }
  exit:
    delete[] formatArray;
    return sts == NV_ENC_SUCCESS;
  }

  bool IsSupportCapacity(const GUID &codec, const NV_ENC_CAPS cap) {
    NV_ENC_CAPS_PARAM capParam{};
    capParam.version = NV_ENC_CAPS_PARAM_VER;
    capParam.capsToQuery = cap;
    int validate = 0;
    NVENCSTATUS sts = m_pNvApi->nvEncGetEncodeCaps(m_hEncSession, codec,
                                                   &capParam, &validate);
    if (sts != NV_ENC_SUCCESS) return false;
    return validate;
  }

  NVENCSTATUS GetDefaultConfig(const GUID &codec, NV_ENC_CONFIG *config) {
    NVENCSTATUS sts;
    uint32_t presetCount, presetSize;
    NV_ENC_PRESET_CONFIG presetConfig{};
    presetConfig.version = NV_ENC_PRESET_CONFIG_VER;
    presetConfig.presetCfg.version = NV_ENC_CONFIG_VER;
    sts =
        m_pNvApi->nvEncGetEncodePresetCount(m_hEncSession, codec, &presetCount);
    if (sts != NV_ENC_SUCCESS) return sts;
    GUID *presetGuidArray = new GUID[presetCount];
    sts = m_pNvApi->nvEncGetEncodePresetGUIDs(
        m_hEncSession, codec, presetGuidArray, presetCount, &presetSize);
    if (sts != NV_ENC_SUCCESS) goto exit;
    for (auto i = 0U; i < presetSize; ++i) {
      if (presetGuidArray[i] == NV_ENC_PRESET_DEFAULT_GUID) {
        sts = m_pNvApi->nvEncGetEncodePresetConfig(
            m_hEncSession, codec, presetGuidArray[i], &presetConfig);
        break;
      }
    }
    if (sts == NV_ENC_SUCCESS) {
      memcpy(config, &presetConfig.presetCfg, sizeof NV_ENC_CONFIG);
    }
  exit:
    delete[] presetGuidArray;
    return sts;
  }

  NVENCSTATUS InitEncoder(NV_ENC_INITIALIZE_PARAMS *config) {
    return m_pNvApi->nvEncInitializeEncoder(m_hEncSession, config);
  }

  NVENCSTATUS RegisterResource(const int width, const int height,
                               const int pitch,
                               const NV_ENC_BUFFER_FORMAT format, void *tex,
                               NV_ENC_REGISTERED_PTR *regHandle) {
    NV_ENC_REGISTER_RESOURCE res{};
    res.version = NV_ENC_REGISTER_RESOURCE_VER;
    res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
    res.resourceToRegister = tex;
    res.bufferFormat = format;
    res.width = width;
    res.height = height;
    res.pitch = pitch;
    NVENCSTATUS sts = m_pNvApi->nvEncRegisterResource(m_hEncSession, &res);
    *regHandle = res.registeredResource;
    return sts;
  }

  NVENCSTATUS UnregisterResource(const NV_ENC_REGISTERED_PTR &res) {
    return m_pNvApi->nvEncUnregisterResource(m_hEncSession, res);
  }

  NVENCSTATUS MapResource(const NV_ENC_REGISTERED_PTR &toMap,
                          NV_ENC_INPUT_PTR *mapped) {
    NV_ENC_MAP_INPUT_RESOURCE mapres{};
    mapres.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    mapres.registeredResource = toMap;
    NVENCSTATUS sts = m_pNvApi->nvEncMapInputResource(m_hEncSession, &mapres);
    *mapped = mapres.mappedResource;
    return sts;
  }

  NVENCSTATUS UnmapResource(const NV_ENC_INPUT_PTR &mapped) {
    return m_pNvApi->nvEncUnmapInputResource(m_hEncSession, mapped);
  }

  NVENCSTATUS CreateBitstreamBuffer(const int size, NV_ENC_OUTPUT_PTR *ptr) {
    NV_ENC_CREATE_BITSTREAM_BUFFER buf{};
    buf.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
    // buf.size = size;
    NVENCSTATUS sts = m_pNvApi->nvEncCreateBitstreamBuffer(m_hEncSession, &buf);
    *ptr = buf.bitstreamBuffer;
    return sts;
  }

  NVENCSTATUS DestroyBitstreamBuffer(const NV_ENC_OUTPUT_PTR &ptr) {
    return m_pNvApi->nvEncDestroyBitstreamBuffer(m_hEncSession, ptr);
  }

  NVENCSTATUS RegisterSyncEvent(HANDLE *syncEvent) {
    NV_ENC_EVENT_PARAMS event{};
    event.version = NV_ENC_EVENT_PARAMS_VER;
    event.completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    *syncEvent = event.completionEvent;
    return m_pNvApi->nvEncRegisterAsyncEvent(m_hEncSession, &event);
  }

  NVENCSTATUS UnregisterSyncEvent(HANDLE syncEvent) {
    NV_ENC_EVENT_PARAMS event{};
    event.version = NV_ENC_EVENT_PARAMS_VER;
    event.completionEvent = syncEvent;
    NVENCSTATUS sts =
        m_pNvApi->nvEncUnregisterAsyncEvent(m_hEncSession, &event);
    CloseHandle(syncEvent);
    return sts;
  }

  NVENCSTATUS LockBitstream(void *pV, void **pS, uint32_t *size) {
    NVENCSTATUS sts;
    NV_ENC_LOCK_BITSTREAM lockBs{};
    lockBs.version = NV_ENC_LOCK_BITSTREAM_VER;
    lockBs.outputBitstream = pV;
    lockBs.doNotWait = true;
    sts = m_pNvApi->nvEncLockBitstream(m_hEncSession, &lockBs);
    if (sts == NV_ENC_SUCCESS) {
      *size = lockBs.bitstreamSizeInBytes;
      *pS = lockBs.bitstreamBufferPtr;
    }
    return sts;
  }

  NVENCSTATUS UnlockBitstream(void *bitstream) {
    return m_pNvApi->nvEncUnlockBitstream(m_hEncSession, bitstream);
  }

  NVENCSTATUS EncodeFrame(NV_ENC_PIC_PARAMS *params) {
    return m_pNvApi->nvEncEncodePicture(m_hEncSession, params);
  }

  NVENCSTATUS GetEncodeStat(NV_ENC_STAT *stat) {
    stat->version = NV_ENC_STAT_VER;
    return m_pNvApi->nvEncGetEncodeStats(m_hEncSession, stat);
  }

 private:
  HANDLE m_hEncSession;
  std::unique_ptr<NV_ENCODE_API_FUNCTION_LIST> m_pNvApi;
};
}  // namespace nvenc

#endif  // LL_CODEC_NVENC_API_NVENCODEAPI$$_H_
