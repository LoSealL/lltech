/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : shape plain
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_PLAIN_H_
#define LL_GRAPHIC_MODEL_PLAIN_H_
#include "ll_graphic/model/model.h"

namespace ll {
namespace engine {
namespace model {
/* A plain */
class Plain : public Model {
 public:
  Plain(float width, float height);

  virtual ~Plain(){};
};
}  // namespace model
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MODEL_PLAIN_H_
