/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : cube
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_CUBE_H_
#define LL_GRAPHIC_MODEL_CUBE_H_
#include <algorithm>
#include <iterator>
#include "ll_graphic/model/model.h"
#include "ll_graphic/model/plain.h"

namespace ll {
namespace engine {
namespace model {
/* A cube */
class Cube : public Model {
 public:
  Cube(float length, float width, float height);

  virtual ~Cube(){};
};
}  // namespace model
}  // namespace engine
}  // namespace ixr

#endif  // LL_GRAPHIC_MODEL_CUBE_H_
