/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : matrix 4x4
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_MATH_MATRIX_H_
#define LL_GRAPHIC_MATH_MATRIX_H_

namespace ll {
namespace engine {
namespace math {
class Quaternion;

class IXR_ENGINE_API Matrix {
 public:
  Matrix();
  Matrix(const float *data);
  Matrix(const Quaternion &q);
  ~Matrix();

  Matrix Mul(const Matrix &rhs);

  Matrix Add(const Matrix &rhs);

  Matrix Transpose() const;

  Matrix Scale(float scalar) const;

  float Det() const;

  Matrix &operator=(const Matrix &rhs);
  Matrix operator+(const Matrix &rhs);
  Matrix operator-(const Matrix &rhs);
  Matrix operator*(const Matrix &rhs);
  Matrix operator*(float scalar);
  float operator()(const int r, const int c) const;

  static Matrix Identity();

 private:
  float m_[16];
};
}  // namespace math
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_MATH_MATRIX_H_
