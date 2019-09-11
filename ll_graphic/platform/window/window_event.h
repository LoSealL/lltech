/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : window events
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_PLATFORM_WINDOW_WINDOW_EVENT_H_
#define LL_GRAPHIC_PLATFORM_WINDOW_WINDOW_EVENT_H_
#include "ll_graphic/engine/types.h"

namespace ll {
namespace engine {

struct WindowEvent {
  uint32_t id;
  uint32_t type;
  uint64_t value[4];
  // A callback function f(id, type, value[4])
  std::function<void(uint32_t, uint32_t, uint64_t*)> callback;
};
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_PLATFORM_WINDOW_WINDOW_EVENT_H_
