/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : delete copy constructors
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/

#ifndef LL_GRAPHIC_PLATFORM_NONCOPYABLE_H_
#define LL_GRAPHIC_PLATFORM_NONCOPYABLE_H_
namespace ll {
/* A base class w/o copy constructors */
class noncopyable {
 protected:
  noncopyable() = default;
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
};
}  // namespace ixr

#endif  // LL_GRAPHIC_PLATFORM_NONCOPYABLE_H_
