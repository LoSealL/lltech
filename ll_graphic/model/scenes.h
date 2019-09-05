/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : scenes
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MODEL_SCENES_H_
#define LL_GRAPHIC_MODEL_SCENES_H_
#include "ll_graphic/core/renderer/renderer.h"
#include "ll_graphic/core/vertex/vertex_buffer.h"
#include "ll_graphic/engine/types.h"
#include "ll_graphic/export/export.h"
#include "ll_graphic/model/camera.h"
#include "ll_graphic/model/model.h"

namespace ll {
namespace engine {
class Env;
namespace model {
/* A scene is a container for a world */
class IXR_ENGINE_API Scene {
 public:
  using ShapeContainer = std::map<String, Model *>;
  using ShapeVertexBuffer = std::map<Model *, core::VertexBuffer>;

  Scene(Env *e);

  ~Scene();

  void AddShape(Model *shape, const char *name = nullptr);

  void RemoveShape(const char *name) noexcept;

  void Draw(core::Renderer *renderer);

 protected:
  ShapeContainer shapes_;
  ShapeVertexBuffer shapebuffers_;
  engine::Env *env_;
};
}  // namespace model
}  // namespace engine
}  // namespace ixr

#endif  // LL_GRAPHIC_MODEL_SCENES_H_
