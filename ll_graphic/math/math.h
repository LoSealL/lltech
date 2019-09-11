/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : math helper
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MATH_MATH_H_
#define LL_GRAPHIC_MATH_MATH_H_
#include "ll_graphic/engine/types.h"
#include "ll_graphic/export/export.h"
#include "ll_graphic/math/matrix.h"
#include "ll_graphic/math/position.h"
#include "ll_graphic/math/rotation.h"

namespace ll {
namespace engine {
namespace math {

constexpr float PI = 3.14159f;
constexpr float EPSILON = 1e-5f;

float IXR_ENGINE_API Angular(const Position &a, const Position &b);

Matrix IXR_ENGINE_API LookToLH(const Position &eye, const Position &at,
                               const Position &up);

Matrix IXR_ENGINE_API PerspectiveLH(float fov, float ratio, float near,
                                    float far);
}  // namespace math
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MATH_MATH_H_
