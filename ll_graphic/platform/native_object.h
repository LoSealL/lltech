/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : basic object
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_PLATFORM_NATIVE_OBJECT_H_
#define LL_GRAPHIC_PLATFORM_NATIVE_OBJECT_H_
namespace ll {
namespace engine {
class Object {
 public:
  void Release() { delete this; }
  virtual ~Object(){};
};
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_PLATFORM_NATIVE_OBJECT_H_
