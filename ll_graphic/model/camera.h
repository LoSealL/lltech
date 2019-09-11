/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : camera helper
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_CAMERA_H_
#define LL_GRAPHIC_MODEL_CAMERA_H_
#include "ll_graphic/math/math.h"
#include "ll_graphic/model/spatial.h"

namespace ll {
namespace engine {
namespace model {
class Camera : public Spatial {
 public:
  Camera();

  virtual ~Camera();

  void LookAt(math::Position point);

  math::Matrix GetProjectionMatrix();
};
}  // namespace model
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MODEL_CAMERA_H_
