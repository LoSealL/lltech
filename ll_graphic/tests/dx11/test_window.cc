#include "ll_graphic/engine/engine.h"
#include <chrono>
#include <gtest/gtest.h>

TEST(window, create_window) {
  using namespace ll::engine;
  Env *e = Env::NewEnvironment(GRAPHIC_API_DX11);
  auto h = e->GetHandle();
  WindowDesc desc{};
  desc.name = "TestWindow";
  desc.style = WINDOW_STYLE_DEFAULT;
  desc.width = 640;
  desc.height = 500;
  auto app = e->NewWindow(desc);
  app->Show();
  while (app->LoopState() != WINDOW_STATE_HALT) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  e->ReleaseObject(app);
  Env::CleanupEnvironment(e);
}
