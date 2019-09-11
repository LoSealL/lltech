/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : error exceptions
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_ERRORS_ERROR_H_
#define LL_GRAPHIC_ERRORS_ERROR_H_
#include <exception>

namespace ll {
namespace engine {
namespace errors {

class Exception : public std::exception {
public:
  template <class... U>
  Exception(int line, const char *func, const char *file, U &&... args) {
    memset(msg_, 0, sizeof msg_);
#ifdef _WIN32
    sprintf_s(msg_, std::forward<U>(args)...);
#else
    sprintf(msg_, std::forward<U>(args)...);
#endif
  }

  ~Exception() throw() {}

protected:
  int line_;
  char msg_[4096];
};
} // namespace errors
} // namespace engine
} // namespace ll

#define THROW_IF_FAIL(expr, ...)                                               \
  if (expr) {                                                                  \
    throw ll::engine::errors::Exception(__LINE__, __FUNCTION__, __FILE__,      \
                                        __VA_ARGS__);                          \
  }

#define ETHROW(...)                                                            \
  throw ll::engine::errors::Exception(__LINE__, __FUNCTION__, __FILE__,        \
                                      __VA_ARGS__)

#endif // LL_GRAPHIC_ERRORS_ERROR_H_
