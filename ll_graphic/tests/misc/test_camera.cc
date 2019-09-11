#include <gtest/gtest.h>
#include "ll_graphic/math/math.h"
#include "ll_graphic/model/camera.h"

using namespace ll::engine::model;

TEST(camera, lookat) {
  Camera c;
  c.LookAt({ 1,0,0 });
  c.LookAt({ 0,0,1 });
  c.SetPosition(0, 0, 0);
  c.Reset();
}
