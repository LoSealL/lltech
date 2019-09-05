/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : model object
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_MODEL_H_
#define LL_GRAPHIC_MODEL_MODEL_H_
#include "ll_graphic/model/spatial.h"
#include "ll_graphic/model/vertex/vertex.h"

namespace ll {
namespace engine {
namespace model {
class Model : public Spatial {
 public:
  using vertices = std::vector<Vertex>;
  using indices = std::vector<uint32_t>;

  Model();
  Model(const vertices &vert, const indices &indx);
  virtual ~Model();

  virtual vertices &GetVertex();

  virtual indices &GetIndice(uint32_t offset = 0);

  virtual math::Position SetPosition(float x, float y, float z);

  virtual math::Quaternion SetRotation(math::Quaternion q);

  virtual void Rotate(math::Quaternion q);

 protected:
  vertices vert_;
  indices indx_;
  indices indx_offset_;
};
}  // namespace model
}  // namespace engine
}  // namespace ixr

#endif  // LL_GRAPHIC_MODEL_MODEL_H_
