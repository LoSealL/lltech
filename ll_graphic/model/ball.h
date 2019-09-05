/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : shape ball
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_BALL_H_
#define LL_GRAPHIC_MODEL_BALL_H_
#include <algorithm>  // for std::min, std::max
#include "ll_graphic/model/model.h"

namespace ll {
namespace engine {
namespace model {
class Ball : public Model {
 public:
  // construct a geo-sphere
  Ball(float radius, int subdivide);
  // construct a sphere
  Ball(float radius, int stack, int slice);
  virtual ~Ball() {}

 private:
  void Subdivide();
};
}  // namespace model
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MODEL_BALL_H_
