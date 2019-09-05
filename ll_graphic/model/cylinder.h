/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : shape cylinder
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Mar. 9th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_CYLINDER_H_
#define LL_GRAPHIC_MODEL_CYLINDER_H_
#include "ll_graphic/model/model.h"

namespace ll {
namespace engine {
namespace model {
class Cylinder : public Model {
 public:
  Cylinder(float bottom_r, float top_r, float h, int stack, int slice);
  virtual ~Cylinder() {}
};
}  // namespace model
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MODEL_CYLINDER_H_
