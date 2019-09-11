/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : Vertex in model
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_VERTEX_VERTEX_H_
#define LL_GRAPHIC_MODEL_VERTEX_VERTEX_H_
#include "ll_graphic/engine/types.h"
#include "ll_graphic/math/math.h"

namespace ll {
namespace engine {
namespace model {
struct float2 {
  float x, y;
};
struct Vertex {
  math::Position pos;
  float2 texcoord;

  Vertex() {}
  Vertex(math::Position p, float2 t) : pos(p), texcoord(t) {}
  Vertex(float x, float y, float z, float u, float v)
      : pos(x, y, z), texcoord{u, v} {}
};
}  // namespace model
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MODEL_VERTEX_VERTEX_H_
