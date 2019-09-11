#include <gtest/gtest.h>
#include "ll_graphic/math/math.h"
#include <DirectXMath.h>

using namespace ll::engine::math;

TEST(position, move) {
  Position p0(1, 2, 3);
  Position p1(4, -5, 6);
  Position p2(-1, -2, -3);
  EXPECT_EQ(p0, -p2);
  EXPECT_EQ(p0 + p1, Position(5, -3, 9));
  EXPECT_EQ(p0 * 2.0f, Position(2, 4, 6));
  p0.Scale(3);
  EXPECT_FLOAT_EQ(p0.X(), 3);
}

TEST(position, length) {
  Position p0(2, 4, 5);
  auto l = p0.Length();
  EXPECT_FLOAT_EQ(l, sqrtf(4 + 16 + 25.f));
}

TEST(quaternion, euler) {
  using namespace DirectX;
  Quaternion q0(1.2f, 0.4f, 0.6f);
  auto q1 = XMQuaternionRotationRollPitchYaw(1.2f, 0.4f, 0.6f);
  EXPECT_FLOAT_EQ(q0.X(), q1.m128_f32[0]);
  EXPECT_FLOAT_EQ(q0.Y(), q1.m128_f32[1]);
  EXPECT_FLOAT_EQ(q0.Z(), q1.m128_f32[2]);
  EXPECT_FLOAT_EQ(q0.W(), q1.m128_f32[3]);
}

TEST(position, angluar) {
  Position a(0, 1, 0);
  Position b(1, 0, 0);
  EXPECT_FLOAT_EQ(a.Length(), 1.0f);
  EXPECT_FLOAT_EQ(Angular(a, b), DirectX::XM_PIDIV2);
}

TEST(position, cross) {
  Position a(0, 1, 0);
  Position b(1, 0, 0);
  auto c = b * a;
  EXPECT_FLOAT_EQ(c.X(), 0);
  EXPECT_FLOAT_EQ(c.Y(), 0);
  EXPECT_FLOAT_EQ(c.Z(), 1);
}

TEST(matrix, init) {
  Matrix m0(Quaternion::Identity());
  auto m1 = Matrix::Identity();
  auto m2 = m0.Mul(m1);
  EXPECT_FLOAT_EQ(m2(0, 0), 1.f);
  EXPECT_FLOAT_EQ(m2(1, 1), 1.f);
  EXPECT_FLOAT_EQ(m2(2, 2), 1.f);
  EXPECT_FLOAT_EQ(m2(3, 3), 1.f);
}

TEST(matrix, mul) {
  float d1[] = {
    1,2,3,4,
    4,3,2,1,
    5,6,7,8,
    8,7,6,5
  };
  float d2[] = {
    4,2,6,5,
    8,5,3,4,
    6,2,8,6,
    4,2,7,6,
  };
  Matrix m1(d1);
  Matrix m2(d2);
  auto m3 = m1 * m2;
  EXPECT_FLOAT_EQ(m3(0, 0), 54.f);
  EXPECT_FLOAT_EQ(m3(2, 0), 142.f);
  EXPECT_FLOAT_EQ(m3(1, 1), 29.f);
  EXPECT_FLOAT_EQ(m3(0, 3), 55.f);
  EXPECT_FLOAT_EQ(m3(1, 3), 50.f);
  EXPECT_FLOAT_EQ(m3(3, 2), 152.f);
}
