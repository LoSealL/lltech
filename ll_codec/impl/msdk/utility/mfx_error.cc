/********************************************************************
Copyright 2017-2018 Intel Corp. All Rights Reserved.
Description
  Intel MediaSDK utilities: error and exception
Author   :    Wenyi Tang
Email    :    wenyi.tang@intel.com
Created  :    Feb. 15th, 2017
Mod      :    Date      Author

********************************************************************/
#include "ll_codec/impl/msdk/utility/mfx_error.h"
#include <iostream>
#include <sstream>


namespace mfxvr {
mfxStatus CheckStatus(mfxStatus sts, std::string msg, const char* file,
                      const int line, mfxStatus apt) {
  CheckStatus(sts, apt, "%s. Error code: [%d], in %s @line %d\n", msg.c_str(),
              sts, file, line);
  return sts;
}

CVRMFXError::CVRMFXError(std::string msg, int err)
    : std::runtime_error(msg), _errcode(err) {}

CVRMFXError::~CVRMFXError() {}

int CVRMFXError::ErrorCode() const { return _errcode; }
}  // namespace mfxvr
