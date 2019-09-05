/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  Intel MediaSDK utilities: error and exception
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_MFX_ERROR_H_
#define LL_CODEC_MFXVR_UTILITY_MFX_ERROR_H_
#include <mfxdefs.h>
#include <stdexcept>
#include <exception>
#include <string>
#include <utility>


namespace mfxvr {
/**
 * \class mfxVR exception, derived from runtime error
 */
class CVRMFXError : public std::runtime_error {
 public:
  CVRMFXError(std::string msg, int err);

  virtual ~CVRMFXError();

  int ErrorCode() const;

 private:
  volatile int _errcode;  ///< error code
};

/**
 * Check for status code, throw if occur any errors.
 * \param [in] sts: mfx status code, \see mfxStatus.
 * \param [in] msg: error explain message.
 * \param [in] file: code source file name.
 * \param [in] line: line number. Will be omitted if line < 0.
 * \param [in] apt: don't throw if sts==apt.
 *
 * \return sts.
 * \throws \class CVRMFXError
 */
mfxStatus CheckStatus(mfxStatus sts, std::string msg,
                      const char* file = nullptr, const int line = -1,
                      mfxStatus apt = MFX_ERR_NONE);

template <class... Args>
mfxStatus CheckStatus(mfxStatus sts, mfxStatus apt, const char* fmt,
                      Args... args) {
  // except for apt
  if (sts == apt) {
    return sts;
  }
  // error
  char buf[1024]{};
  snprintf(buf, sizeof buf, fmt, std::forward<Args>(args)...);
  if (sts < 0) {
    throw CVRMFXError(buf, sts);
  }
  // warning
  if (sts > 0) {
    // do nothing
  }
  return sts;
}
}  // namespace mfxvr
#endif  // LL_CODEC_MFXVR_UTILITY_MFX_ERROR_H_
