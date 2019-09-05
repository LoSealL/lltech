/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description : NV encoder framework
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Nov. 11th, 2017
changelog
********************************************************************/
#ifndef LL_CODEC_NVENC_NV_ERROR_H_
#define LL_CODEC_NVENC_NV_ERROR_H_
#include <exception>
#include <string>
#include "ll_codec/impl/nvenc/api/nvEncodeAPI++.h"


namespace nvenc {

template <class... Args>
NVENCSTATUS CheckStatus(NVENCSTATUS sts, NVENCSTATUS apt, const char* fmt,
                        Args... args) {
  // except for apt
  if (sts == apt) {
    return sts;
  }
  // error
  char buf[1024]{};
  snprintf(buf, sizeof buf, fmt, std::forward<Args>(args)...);
  if (sts != NV_ENC_SUCCESS) {
    throw CVRNvException(buf, sts);
  }
  return sts;
}

/**
 * Check for status code, throw if occur any errors.
 * \param [in] sts: nvenc status code, \see NVENCSTATUS.
 * \param [in] msg: error explain message.
 * \param [in] file: code source file name.
 * \param [in] line: line number. Will be omitted if line < 0.
 * \param [in] apt: don't throw if sts==apt.
 *
 * \return sts.
 * \throws \class CVRNvException
 */
inline NVENCSTATUS CheckStatus(NVENCSTATUS sts, std::string msg,
                               const char* file = nullptr, const int line = -1,
                               NVENCSTATUS apt = NV_ENC_SUCCESS) {
  CheckStatus(sts, apt, "%s. Error code: [%d], in %s @line %d\n", msg.c_str(),
              sts, file, line);
  return sts;
}

/**
 * \class NVENC exception, derived from runtime error
 */
class CVRNvException : public std::runtime_error {
 public:
  CVRNvException(std::string msg, int err)
      : std::runtime_error(msg), _errcode(err) {}

  virtual ~CVRNvException() {}

 private:
  volatile int _errcode;  ///< error code
};
}  // namespace nvenc

#define CHECK_STATUS(sts, msg) CheckStatus(sts, msg, __FILE__, __LINE__)

#endif  // LL_CODEC_NVENC_NV_ERROR_H_
