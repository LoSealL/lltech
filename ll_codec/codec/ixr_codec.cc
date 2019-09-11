/********************************************************************
Copyright 2017-2018 Tang, Wenyi. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 14th, 2017
changelog
********************************************************************/
#include "ll_codec/codec/ixr_codec.h"
#include "ll_codec/codec/ixr_codec_impl.h"

std::unique_ptr<ixr::Encoder> ixr::Encoder::Create(ixr::AdapterVendor vid) {
  switch (vid) {
    case IXR_CODEC_VID_INTEL:
      return std::make_unique<ixr::EncoderImplIntel>();
      break;
    case IXR_CODEC_VID_NVIDIA:
      return std::make_unique<ixr::EncoderImplNvidia>();
      break;
    case IXR_CODEC_VID_AMD:
      return nullptr;
      break;
    case IXR_CODEC_VID_MSVC_DEBUGGER:
      return nullptr;
      break;
    default:
      break;
  }
  return nullptr;
}

std::unique_ptr<ixr::Decoder> ixr::Decoder::Create(ixr::AdapterVendor vid) {
  switch (vid) {
    case IXR_CODEC_VID_INTEL:
      return std::make_unique<ixr::DecoderImplIntel>();
      break;
    case IXR_CODEC_VID_NVIDIA:
      return nullptr;
      break;
    case IXR_CODEC_VID_AMD:
      return nullptr;
      break;
    case IXR_CODEC_VID_MSVC_DEBUGGER:
      return nullptr;
      break;
    default:
      break;
  }
  return nullptr;
}

std::unique_ptr<ixr::Vpp> ixr::Vpp::Create(ixr::AdapterVendor vid) {
  switch (vid) {
    case IXR_CODEC_VID_INTEL:
      return std::make_unique<ixr::VppImplIntel>();
      break;
    case IXR_CODEC_VID_NVIDIA:
      return nullptr;
      break;
    case IXR_CODEC_VID_AMD:
      return nullptr;
      break;
    case IXR_CODEC_VID_MSVC_DEBUGGER:
      return nullptr;
      break;
    default:
      break;
  }
  return nullptr;
}

std::shared_ptr<ixr::Encoder> ixr::Encoder::Create(ConfigInfo& info) {
  std::shared_ptr<ixr::Encoder> p;
  if (info.vid == IXR_CODEC_VID_INTEL) {
    p.reset(new ixr::EncoderImplIntel());
  } else if (info.vid == IXR_CODEC_VID_NVIDIA) {
    p.reset(new ixr::EncoderImplNvidia());
  }
  if (p) {
    p->Allocate(*info.config);
  }
  return p;
}

std::shared_ptr<ixr::Decoder> ixr::Decoder::Create(ConfigInfo& info) {
  std::shared_ptr<ixr::Decoder> p;
  if (info.vid == IXR_CODEC_VID_INTEL) {
    p.reset(new ixr::DecoderImplIntel());
  }
  if (p) {
    p->Allocate(*info.config, info.nalu, info.nalu_size);
  }
  return p;
}

std::shared_ptr<ixr::Vpp> ixr::Vpp::Create(ConfigInfo& info) {
  std::shared_ptr<ixr::Vpp> p;
  if (info.vid == IXR_CODEC_VID_INTEL) {
    p.reset(new ixr::VppImplIntel());
  }
  if (p) {
    p->Allocate(*info.config);
  }
  return p;
}

namespace ixr {
// dummy class implementation
Encoder::~Encoder() {}
void Encoder::Allocate(const CodecConfig&) {}
void Encoder::Deallocate() {}
CodecStat Encoder::GetEncodeStatus() { return CodecStat(); }
void* Encoder::DequeueInputBuffer() { return nullptr; }
int Encoder::QueueInputBuffer(void*) { return -1; }
int Encoder::QueueUserData(void*, uint32_t) { return -1; }
int Encoder::DequeueUserData(void*, uint32_t*) { return -1; }
int Encoder::DequeueOutputBuffer(void**, uint32_t*) { return -1; }
void Encoder::ReleaseOutputBuffer(void*) {}
void Encoder::GetFlowControlParam(float*, uint32_t*) const {}
void Encoder::SetFlowControlParam(const float, const uint32_t) {}

Decoder::~Decoder() {}
void Decoder::Allocate(CodecConfig&, void*, uint32_t) {}
void Decoder::Deallocate() {}
CodecStat Decoder::GetDecodeStatus() { return CodecStat(); }
int Decoder::QueueInputBuffer(void*, uint32_t) { return -1; }
int Decoder::DequeueOutputBuffer(void**) { return -1; }
void Decoder::ReleaseOutputBuffer(void*) {}
void Decoder::GetPrivateData(void*) const {}
void Decoder::SetPrivateData(void*) {}

Vpp::~Vpp() {}
void Vpp::Allocate(const CodecConfig&) {}
void Vpp::Deallocate() {}
void* Vpp::DequeueInputBuffer() { return nullptr; }
int Vpp::QueueInputBuffer(void*) { return -1; }
int Vpp::DequeueOutputBuffer(void**, uint32_t*) { return -1; }
void Vpp::ReleaseOutputBuffer(void*) {}
}  // namespace ixr