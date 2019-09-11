/********************************************************************
Copyright 2017 Tang, Wenyi. All Rights Reserved.
Description : IXR codec interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 14th, 2017
changelog
********************************************************************/
#include "ll_codec/codec/ixr_codec.h"
#include "res.h"
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

using namespace ixr;

class IntelCodecTest : public ::testing::Test {
protected:
  virtual ixr::CodecConfig GetConfig() {
    ixr::CodecConfig par{};
    par.width = kWidth;
    par.height = kHeight;
    par.bitrate = 150000;
    par.rcMode = ixr::IXR_RC_MODE_VBR;
    par.fps = 30;
    par.gop = 1;
    par.adapter = ixr::IXR_CODEC_VID_INTEL;
    par.asyncDepth = 3;
    par.outputSizeMax = 16 << 20;
    par.advanced.enableIntraRefresh = 0;
    par.advanced.enableMvc = 0;
    return par;
  }
};

inline void LogOutput(const char *name, void *src, int len) {
  std::ofstream f(name, std::ios::binary);
  if (f.is_open()) {
    f.write(static_cast<char *>(src), len);
  }
  f.close();
}

TEST_F(IntelCodecTest, H264EncodeFromCpuNV12) {
  auto par = GetConfig();
  par.codec = ixr::IXR_CODEC_AVC;
  par.inputFormat = ixr::IXR_COLOR_NV12;
  par.memoryType = ixr::IXR_MEM_INTERNAL_CPU;
  Encoder::ConfigInfo info;
  info.vid = par.adapter;
  info.config = &par;
  auto codec = ixr::Encoder::Create(info);
  void *ptr = codec->DequeueInputBuffer();
  memcpy(ptr, sFrameNV12, sizeof sFrameNV12);
  EXPECT_EQ(codec->QueueInputBuffer(nullptr), 0) << "QueueInput Failed";
  void *buf = nullptr;
  uint32_t len = 0;
  EXPECT_EQ(codec->DequeueOutputBuffer(&buf, &len), 0)
      << "DequeueOutput Failed";
  LogOutput("test_encode_nv12_intel_cpu.264", buf, len);
  codec->ReleaseOutputBuffer(buf);
}

TEST_F(IntelCodecTest, H264EncodeFromCpuRGB4) {
  auto par = GetConfig();
  par.codec = ixr::IXR_CODEC_AVC;
  par.inputFormat = ixr::IXR_COLOR_ARGB;
  par.memoryType = ixr::IXR_MEM_INTERNAL_CPU;
  Encoder::ConfigInfo info;
  info.vid = par.adapter;
  info.config = &par;
  auto codec = ixr::Encoder::Create(info);
  void *ptr = codec->DequeueInputBuffer();
  memcpy(ptr, sFrame, sizeof sFrame);
  codec->QueueInputBuffer(nullptr);
  void *buf = nullptr;
  uint32_t len = 0;
  EXPECT_EQ(0, codec->DequeueOutputBuffer(&buf, &len));
  EXPECT_GT(len, 0U);
  LogOutput("test_encode_rgb4_intel_cpu.264", buf, len);
  codec->ReleaseOutputBuffer(buf);
}

TEST_F(IntelCodecTest, H264DecodeIntoCpuRGB4) {
  ixr::CodecConfig par{};
  par.codec = ixr::IXR_CODEC_AVC;
  par.adapter = ixr::IXR_CODEC_VID_INTEL;
  par.memoryType = ixr::IXR_MEM_INTERNAL_CPU;
  par.outputFormat = ixr::IXR_COLOR_ARGB;
  Decoder::ConfigInfo info;
  info.vid = par.adapter;
  info.config = &par;
  info.nalu = const_cast<char *>(sBitstream);
  info.nalu_size = sizeof sBitstream;
  auto codec = ixr::Decoder::Create(info);
  int32_t w, h;
  w = par.width;
  h = par.height;
  EXPECT_EQ(w, kWidth);
  EXPECT_EQ(h, kHeight);
  std::thread t0([&]() {
    void *tex[2]{};
    for (; tex[0] == nullptr;) {
      codec->DequeueOutputBuffer(tex);
    }
    LogOutput("test_decode_h264_intel_cpu.rgb4", tex[0], w * h * 4);
    codec->ReleaseOutputBuffer(tex[0]);
  });
  int ret;
  do {
    ret = codec->QueueInputBuffer(const_cast<char *>(sBitstream),
                                  sizeof sBitstream);
  } while (ret);
  EXPECT_EQ(codec->QueueInputBuffer(0, 0), 0);
  t0.join();
}

TEST_F(IntelCodecTest, H264DecodeIntoCpuNV12) {
  ixr::CodecConfig par{};
  par.codec = ixr::IXR_CODEC_AVC;
  par.adapter = ixr::IXR_CODEC_VID_INTEL;
  par.memoryType = ixr::IXR_MEM_INTERNAL_CPU;
  par.outputFormat = ixr::IXR_COLOR_NV12;
  Decoder::ConfigInfo info;
  info.vid = par.adapter;
  info.config = &par;
  info.nalu = const_cast<char *>(sBitstream);
  info.nalu_size = sizeof sBitstream;
  auto codec = ixr::Decoder::Create(info);
  int32_t w, h;
  w = par.width;
  h = par.height;
  EXPECT_EQ(w, kWidth);
  EXPECT_EQ(h, kHeight);
  std::thread t0([&]() {
    void *tex[2]{};
    for (; tex[0] == nullptr;) {
      codec->DequeueOutputBuffer(tex);
    }
    LogOutput("test_decode_h264_intel_cpu.nv12", tex[0], w * h * 3 / 2);
    codec->ReleaseOutputBuffer(tex[0]);
  });
  int ret;
  do {
    ret = codec->QueueInputBuffer(const_cast<char *>(sBitstream),
                                  sizeof sBitstream);
  } while (ret);
  EXPECT_EQ(codec->QueueInputBuffer(0, 0), 0);
  t0.join();
}

TEST_F(IntelCodecTest, JpegDecodeIntoCpuRGB4) {
  ixr::CodecConfig par{};
  par.codec = ixr::IXR_CODEC_JPEG;
  par.adapter = ixr::IXR_CODEC_VID_INTEL;
  par.memoryType = ixr::IXR_MEM_INTERNAL_CPU;
  par.outputFormat = ixr::IXR_COLOR_ARGB;
  char *src = const_cast<char *>(JPEG::sPicJpeg);
  uint32_t size = sizeof JPEG::sPicJpeg;
  Decoder::ConfigInfo info;
  info.vid = par.adapter;
  info.config = &par;
  info.nalu = src;
  info.nalu_size = size;
  auto codec = ixr::Decoder::Create(info);
  uint32_t w, h;
  w = par.width;
  h = par.height;
  std::thread t0([&]() {
    void *tex[2]{};
    for (; tex[0] == nullptr;) {
      codec->DequeueOutputBuffer(tex);
    }
    codec->ReleaseOutputBuffer(tex[0]);
  });
  EXPECT_NO_THROW(codec->QueueInputBuffer(src, size));
  EXPECT_EQ(w, JPEG::kWidth);
  EXPECT_EQ(h, JPEG::kHeight);
  t0.join();
}

TEST_F(IntelCodecTest, JpegDecodeIntoCpuNV12) {
  ixr::CodecConfig par{};
  par.codec = ixr::IXR_CODEC_JPEG;
  par.adapter = ixr::IXR_CODEC_VID_INTEL;
  par.memoryType = ixr::IXR_MEM_INTERNAL_CPU;
  par.outputFormat = ixr::IXR_COLOR_NV12;
  auto codec = ixr::Decoder::Create(ixr::IXR_CODEC_VID_INTEL);
  char *src = const_cast<char *>(JPEG::sPicJpeg);
  uint32_t size = sizeof JPEG::sPicJpeg;
  codec->Allocate(par, src, size);
  uint32_t w, h;
  w = par.width;
  h = par.height;
  std::thread t0([&]() {
    void *tex[2]{};
    for (; tex[0] == nullptr;) {
      codec->DequeueOutputBuffer(tex);
    }
    codec->ReleaseOutputBuffer(tex[0]);
  });
  EXPECT_NO_THROW(codec->QueueInputBuffer(src, size));
  EXPECT_EQ(w, JPEG::kWidth);
  EXPECT_EQ(h, JPEG::kHeight);
  t0.join();
}

TEST(VppCpu, Resize) {
  ixr::CodecConfig par{};
  par.asyncDepth = 2;
  par.width = kWidth;
  par.height = kHeight;
  par.inputFormat = ixr::IXR_COLOR_NV12;
  par.vpp.inCrop[0] = 0;
  par.vpp.inCrop[1] = 0;
  par.vpp.inCrop[2] = kWidth;
  par.vpp.inCrop[3] = kHeight;
  par.outputFormat = ixr::IXR_COLOR_NV12;
  par.vpp.outWidth = kWidth * 2;
  par.vpp.outHeight = kHeight * 2;
  par.vpp.outCrop[0] = 0;
  par.vpp.outCrop[1] = 0;
  par.vpp.outCrop[2] = kWidth * 2;
  par.vpp.outCrop[3] = kHeight * 2;
  Vpp::ConfigInfo info;
  info.vid = IXR_CODEC_VID_INTEL;
  info.config = &par;
  auto vpp = ixr::Vpp::Create(info);
  auto ptr = vpp->DequeueInputBuffer();
  memcpy(ptr, sFrameNV12, kWidth * kHeight * 3 / 2);
  auto ret = vpp->QueueInputBuffer(ptr);
  EXPECT_EQ(ret, 0);
  ret = vpp->DequeueOutputBuffer(&ptr, nullptr);
  EXPECT_EQ(ret, 0);
  vpp->ReleaseOutputBuffer(ptr);
  if (ret == 0) {
    LogOutput("test-vpp-resize.nv12", ptr, kWidth * kHeight * 6);
  }
}

TEST(VPP, DequeueEagerly) {
  ixr::CodecConfig par{};
  par.asyncDepth = 2;
  par.width = kWidth;
  par.height = kHeight;
  par.inputFormat = ixr::IXR_COLOR_NV12;
  par.vpp.inCrop[0] = 0;
  par.vpp.inCrop[1] = 0;
  par.vpp.inCrop[2] = kWidth;
  par.vpp.inCrop[3] = kHeight;
  par.outputFormat = ixr::IXR_COLOR_ARGB;
  par.vpp.outWidth = kWidth * 2;
  par.vpp.outHeight = kHeight * 2;
  par.vpp.outCrop[0] = 0;
  par.vpp.outCrop[1] = 0;
  par.vpp.outCrop[2] = kWidth * 2;
  par.vpp.outCrop[3] = kHeight * 2;
  const uint32_t kPitchNV12 = kWidth * kHeight * 3 / 2;
  Vpp::ConfigInfo info;
  info.vid = IXR_CODEC_VID_INTEL;
  info.config = &par;
  auto vpp = ixr::Vpp::Create(info);
  std::vector<void *> buf;
  bool exit = false;
  int count[2]{};
  std::thread t1([&vpp, &count, &exit]() {
    void *ptr = nullptr;
    const uint32_t kPitchNV12 = kWidth * kHeight * 3 / 2;
    do {
      ptr = vpp->DequeueInputBuffer();
      if (ptr) {
        memcpy(ptr, sFrameNV12, kPitchNV12);
        if (vpp->QueueInputBuffer(ptr) == 0) {
          count[0]++;
        }
      }
    } while (!exit);
  });

  std::thread t2([&vpp, &count, &exit]() {
    void *ptr = nullptr;
    for (;;) {
      int ret = vpp->DequeueOutputBuffer(&ptr, nullptr);
      if (ret == 0) {
        vpp->ReleaseOutputBuffer(ptr);
        count[1]++;
      }
      if (count[1] == count[0])
        break;
    }
  });
  std::this_thread::sleep_for(std::chrono::seconds(1));
  exit = true;
  t1.join();
  t2.join();
  vpp->Deallocate();
}
