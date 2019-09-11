#include "ll_graphic/engine/engine.h"
#include "ll_graphic/model/scenes.h"
#include <gtest/gtest.h>
using namespace ll::engine;

TEST(vulkan, create_renderer) {
#if defined(DEBUG) | defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
  auto env = Env::NewEnvironment(GRAPHIC_API_VULKAN);
  Env::Advanced::SetLogLevel(0);
  auto renderer = env->NewRenderer();
  Env::CleanupEnvironment(env);
  return 0;
}