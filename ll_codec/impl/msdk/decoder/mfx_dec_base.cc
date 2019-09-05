/********************************************************************
Copyright 2016-2018 Intel Corp. All Rights Reserved.

Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Mar. 27th, 2017
Mod      :    Date      Author

Using MediaSDK in Android as the codec
********************************************************************/
#include "ll_codec/impl/msdk/decoder/mfx_dec_base.h"
#include <assert.h>
#include <mfxcommon.h>
#include <mfxstructures.h>
#include <mfxvideo.h>
#include <cstring>
#include <thread>
#include "ll_codec/impl/msdk/decoder/jpeg_helper.h"
#if _WIN32
#include "ll_codec/impl/msdk/utility/mfx_alloc_d3d.h"
#endif
#include "ll_codec/impl/msdk/utility/mfx_alloc_sys.h"

namespace mfxvr {
namespace dec {
// Todo config me.
constexpr mfxU32 kAllocBytes = 16 << 20;

CVRDecBase::CVRDecBase() {
  std::memset(&responce_, 0, sizeof(responce_));
  std::memset(&input_bytes_, 0, sizeof(input_bytes_));
  std::memset(&cached_bytes_, 0, sizeof(cached_bytes_));
  input_bytes_.MaxLength = kAllocBytes;
  cached_bytes_.MaxLength = kAllocBytes;
  cached_bytes_.Data = new mfxU8[kAllocBytes];
  old_offset_ = 0;
  vpp_.reset(new vpp::VppChain());
  initSession();
}

CVRDecBase::~CVRDecBase() { delete[] cached_bytes_.Data; }

void CVRDecBase::Config(vrpar::config *par, mfxU8 *header, mfxU32 hsize) {
  mfxStatus sts;
  input_bytes_.Data = header;
  input_bytes_.DataLength = hsize;
  // try parse header
  this->initParameters(par);
  this->initAllocator(par);
  this->initFrames(par);
  // init decoder
  // this will call Alloc::AllocFrames again, with the "same" request
  // and the "same" response should be returned
  sts = mfx_dec_->Init(&video_params_);
  CheckStatus(sts, "Dec->Init", __FILE__, __LINE__);
  sts = mfx_dec_->GetVideoParam(&video_params_);
  CheckStatus(sts, "GetVideoParam", __FILE__, __LINE__);
}

bool CVRDecBase::InputAvaiable() {
  // For all surfaces, it's free if it is
  // 1. not locked by msdk and
  // 2. not used after outputs
  return !getFreeSurface(1).empty();
}

bool CVRDecBase::QueueInput(void *src, mfxU32 size) {
  mfxStatus sts = MFX_ERR_NONE;
  const bool kVppUsed = vpp_->VppChainSize() > 0;
  input_bytes_.Data = static_cast<mfxU8 *>(src);
  input_bytes_.DataOffset = old_offset_;
  input_bytes_.DataLength = size - old_offset_;
  mfxBitstream *inp = &input_bytes_;
  if (cached_bytes_.DataLength > 0) {
    // concatenate cached bytes
    auto cache_length_total =
        cached_bytes_.DataLength + cached_bytes_.DataOffset;
    if (cached_bytes_.MaxLength - cache_length_total <
        input_bytes_.DataLength) {
      // not enough cache space for memcpy
      memmove(cached_bytes_.Data + cache_length_total,
              input_bytes_.Data + input_bytes_.DataOffset,
              cached_bytes_.MaxLength - cache_length_total);
      old_offset_ = input_bytes_.DataLength + cache_length_total -
                    cached_bytes_.MaxLength;
    } else {
      memmove(cached_bytes_.Data + cache_length_total,
              input_bytes_.Data + input_bytes_.DataOffset,
              input_bytes_.DataLength);
      cached_bytes_.DataLength += input_bytes_.DataLength;
      old_offset_ = 0;
    }
    inp = &cached_bytes_;
  } else {
    old_offset_ = 0;
  }
  for (;;) {
    // get unlocked work surface
    auto workers = getFreeSurface(1);
    if (workers.empty()) {
      // not enough free surface, return false to tell caller
      // to queue in the same buffer once again.
      if (inp == &cached_bytes_) {
        return old_offset_ == 0;
      } else {
        old_offset_ = inp->DataOffset;
        return false;
      }
    }
    mfxFrameSurface1 *worker = workers[0];
    mfxFrameSurface1 *outp;
    mfxSyncPoint sync;
    for (;;) {
      if (!src && size == 0) {
        // signal EOF
        sts = mfx_dec_->DecodeFrameAsync(nullptr, worker, &outp, &sync);
      } else {
        sts = mfx_dec_->DecodeFrameAsync(inp, worker, &outp, &sync);
      }
      if (sts == MFX_WRN_DEVICE_BUSY || sts == MFX_WRN_VIDEO_PARAM_CHANGED) {
        std::this_thread::yield();
      } else {
        break;
      }
    }
    if (sts == MFX_ERR_NONE) {
      worker_status_[outp].inuse = true;
      worker_status_[outp].surf = outp;
      if (kVppUsed) {
        mfxFrameSurface1 *vpp_outp;
        sts = vpp_->RunVpp1(outp, &vpp_outp, &sync);
        CheckStatus(sts, "RunFrameVPPAsync", __FILE__, __LINE__);
        worker_status_[outp].surf = vpp_outp;
      }
      worker_status_[outp].sync = sync;
      outputs_.Push(outp);
    } else if (sts != MFX_ERR_MORE_SURFACE) {
      break;
    }
    if (inp->DataLength == 0) break;
  }
  CheckStatus(sts, "DecodeFrameAsync", __FILE__, __LINE__, MFX_ERR_MORE_DATA);
  // MSDK sometimes return no error with a little bytes remained unprocessed.
  // We have to cache these bytes and concatenate with future bytes afterward.
  if (inp->DataLength > 0) {
    std::memmove(cached_bytes_.Data, inp->Data + inp->DataOffset, inp->DataLength);
    cached_bytes_.DataLength = inp->DataLength;
    cached_bytes_.DataOffset = 0;
  }
  return old_offset_ == 0;
}

void CVRDecBase::DequeueOutputSurface(void **surface) {
  mfxFrameSurface1 *outputhead;
  if (outputs_.TryPop(&outputhead)) {
    mfxSyncPoint sync = worker_status_[outputhead].sync;
    mfxFrameSurface1 *surf = worker_status_[outputhead].surf;
    assert(worker_status_[outputhead].inuse);
    mfxStatus sts = sess_.SyncOperation(sync, MFX_INFINITE);
    CheckStatus(sts, "SyncOperation", __FILE__, __LINE__);
    sts = allocator_->GetHDL(allocator_->pthis, surf->Data.MemId, surface);
    CheckStatus(sts, "GetHDL", __FILE__, __LINE__);
    std::lock_guard<std::mutex> locker(release_mutex_);
    release_tab_[*surface] = outputhead;
  } else {
    *surface = nullptr;
    return;
  }
}

void CVRDecBase::ReleaseOutputSurface(void *surface) {
  std::lock_guard<std::mutex> locker(release_mutex_);
  mfxFrameSurface1 *frame = release_tab_.at(surface);
  if (worker_status_.find(frame) != worker_status_.end()) {
    worker_status_[frame].inuse = false;
    worker_status_[frame].sync = 0;
    vpp_->ReleaseSurface(worker_status_[frame].surf);
  }
  release_tab_.erase(surface);
}

void CVRDecBase::initSession() {
  // The version has to be "1.0" to successfully init mfx.
  mfxVersion version{0, 1};
  mfxInitParam par{};
  par.Implementation = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D11;
  par.GPUCopy = MFX_GPUCOPY_DEFAULT;
  par.Version = version;
  // Try to use HW implementation first, then software impl.
  mfxStatus sts = sess_.InitEx(par);
  if (sts != MFX_ERR_NONE) {
    par.Implementation = MFX_IMPL_SOFTWARE;
    sts = sess_.InitEx(par);
  }
  CheckStatus(sts, "InitEx", __FILE__, __LINE__);
  // Require API >= v1.18
  mfxVersion ver{};
  sts = sess_.QueryVersion(&ver);
  if (ver.Minor < 18) sts = MFX_ERR_UNSUPPORTED;
  CheckStatus(sts, MFX_ERR_NONE,
              "- Version unsupported: ver %d.%d, minimum required 1.18",
              ver.Major, ver.Minor);
  // construct decoder
  mfx_dec_.reset(new MFXVideoDECODE(sess_));
}

void CVRDecBase::initParameters(vrpar::config *par) {
  mfxStatus sts;
  std::memset(&video_params_, 0, sizeof(video_params_));
  video_params_.mfx.CodecId = par->codec;
  // ???
  video_params_.AsyncDepth = 1;
  if (par->codec == MFX_CODEC_JPEG) {
    MJPEG_AVI_ParsePicStruct(&input_bytes_);
    input_bytes_.DataFlag |= MFX_BITSTREAM_EOS;
  }
  sts = mfx_dec_->DecodeHeader(&input_bytes_, &video_params_);
  CheckStatus(sts, "DecodeHeader", __FILE__, __LINE__);
  par->out.width = par->in.width = video_params_.mfx.FrameInfo.Width;
  par->out.height = par->in.height = video_params_.mfx.FrameInfo.Height;
  par->in.cropW = par->out.cropW = video_params_.mfx.FrameInfo.Width;
  par->in.cropH = par->out.cropH = video_params_.mfx.FrameInfo.Height;
  par->in.color_format = video_params_.mfx.FrameInfo.FourCC;
  // deal with mvc
  if (par->multiViewCodec) {
    ext_mvc_.reset(new CMVCExt());
    external_buff_.push_back(ext_mvc_->getAddressOf());
    video_params_.ExtParam = &external_buff_[0];
    video_params_.NumExtParam = (mfxU16)external_buff_.size();
    sts = mfx_dec_->DecodeHeader(&input_bytes_, &video_params_);
    if (sts == MFX_ERR_NOT_ENOUGH_BUFFER) {
      ext_mvc_->AllocMVCBuffer();
      sts = mfx_dec_->DecodeHeader(&input_bytes_, &video_params_);
    }
    CheckStatus(sts, "- Error in DecodeHeader MVC", __FILE__, __LINE__);
  }
  if (par->codec == MFX_CODEC_JPEG && (input_bytes_.PicStruct & 0x6)) {
    // TFF or BFF jpeg stream
    video_params_.mfx.FrameInfo.CropH *= 2;
    video_params_.mfx.FrameInfo.Height =
        MAKE_ALIGN16(video_params_.mfx.FrameInfo.CropH);
    video_params_.mfx.FrameInfo.PicStruct = input_bytes_.PicStruct;
  }
  if (par->codec == MFX_CODEC_JPEG &&
      par->out.color_format == MFX_FOURCC_RGB4) {
    video_params_.mfx.FrameInfo.FourCC = MFX_FOURCC_RGB4;
    video_params_.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
    // If output format is RGB, the AVC/MVC need VPP to CSC
    // JPEG doesn't need VPP (assume we don't need other vpp)
    par->in.color_format = MFX_FOURCC_RGB4;
    // @todo: mfx.JPEGColorFormat be explicit valued?
  }
}

void CVRDecBase::initAllocator(vrpar::config *par) {
  mfxStatus sts = MFX_ERR_NONE;
  mfxHDL hdl = par->renderer;
  if (!hdl) {
    video_params_.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    allocator_.reset(new CVRSysAllocator());
  } else {
    video_params_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
#ifdef _WIN32
    sts = sess_.SetHandle(MFX_HANDLE_D3D11_DEVICE, hdl);
    if (sts != MFX_ERR_NONE) {
      // Set multiple thread protected for MFX
      ID3D11Device *dxhdl = reinterpret_cast<ID3D11Device *>(hdl);
      ID3D10Multithread *pmt = nullptr;
      HRESULT hr = dxhdl->QueryInterface(&pmt);
      if (SUCCEEDED(hr)) {
        pmt->SetMultithreadProtected(true);
        sts = sess_.SetHandle(MFX_HANDLE_D3D11_DEVICE, hdl);
        pmt->Release();
      }
    }
    CheckStatus(sts, "- Error in SetHandle (Dx)", __FILE__, __LINE__);
    allocator_.reset(new CVRDX11Allocator(hdl));
#endif
  }
  sts = sess_.SetFrameAllocator(allocator_.get());
  CheckStatus(sts, "SetFrameAllocator", __FILE__, __LINE__);
}

void CVRDecBase::initFrames(vrpar::config *par) {
  mfxStatus sts;
  mfxFrameAllocRequest request{}, vppreq[2]{};
  // query video parameters
  sts = mfx_dec_->Query(&video_params_, &video_params_);
  CheckStatus(sts, "Dec->Query", __FILE__, __LINE__);
  sts = mfx_dec_->QueryIOSurf(&video_params_, &request);
  if (request.NumFrameSuggested < video_params_.AsyncDepth)
    sts = MFX_ERR_MEMORY_ALLOC;
  // A marker
  request.Type |= MFX_MEMTYPE_VR_SPECIAL;
  CheckStatus(sts, "Dec->QueryIOSurf", __FILE__, __LINE__);
  vrpar::config vpp_par = *par;
  vpp_par.asyncDepth = request.NumFrameSuggested + 2;
  vpp_->Alloc(sess_, allocator_.get(), vpp_par);
  if (vpp_->VppChainSize() > 0) {
    request.Type |= MFX_MEMTYPE_FROM_VPPIN;
  }
  sts = allocator_->Alloc(allocator_->pthis, &request, &responce_);
  CheckStatus(sts, "Dec->Alloc", __FILE__, __LINE__);
  // allocate mfx surfaces and link to resp->memid
  workers_.resize(responce_.NumFrameActual);
  mfxU16 i = 0;
  for (auto &surf : workers_) {
    worker_status_[&surf] = {};
    std::memset(&surf, 0, sizeof surf);
    surf.Info = request.Info;
    if (par->renderer) {
      surf.Data.MemId = responce_.mids[i++];
    } else {
      allocator_->Lock(allocator_->pthis, responce_.mids[i++], &surf.Data);
    }
  }
}

std::vector<mfxFrameSurface1 *> CVRDecBase::getFreeSurface(int num) {
  std::vector<mfxFrameSurface1 *> freed;
  for (auto &surf : workers_) {
    if (surf.Data.Locked == 0 && !worker_status_[&surf].inuse) {
      freed.push_back(&surf);
    }
    if (freed.size() == num) break;
  }
  return freed;
}

}  // namespace dec
}  // namespace mfxvr
