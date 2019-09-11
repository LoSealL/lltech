#include "ll_graphic/engine/engine.h"
#include "ll_graphic/model/ball.h"
#include "ll_graphic/model/cube.h"
#include "ll_graphic/model/cylinder.h"
#include "ll_graphic/model/plain.h"
#include "ll_graphic/model/scenes.h"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

using namespace ll::engine;
using namespace ll::engine::core;
using namespace ll::engine::math;

TEST(engine, core) {
  auto env = Env::NewEnvironment(GRAPHIC_API_DX11);
  Env::Advanced::SetLogLevel(0);
  try {
    WindowDesc desc{};
    desc.name = "TestCore";
    desc.style = WINDOW_STYLE_DEFAULT;
    desc.width = 640;
    desc.height = 500;
    auto app = env->NewWindow(desc);
    app->Show();

    auto renderer = env->NewRenderer();

    SwapChainDesc sd;
    sd.format = PF_RGBA32_UNORM;
    sd.width = 0;
    sd.height = 0;
    sd.sample_count = 1;
    sd.window_handle = app->GetHandle();
    auto swapchain = env->NewSwapChain(sd);

    Texture *rt = swapchain->GetBuffer(0);

    VertexElement v1("POSITION", VET_FLOAT3), v2("TEXCOORD", VET_FLOAT2);
    auto vs = env->NewShader();
    auto ps = env->NewShader();
    vs->Compile(SHADER_TARGET_VERTEX);
    ps->Compile(SHADER_TARGET_PIXEL);
    renderer->SetInputLayout({v1, v2}, vs);
    renderer->SetShader(vs);
    renderer->SetShader(ps);
    renderer->SetRasterizer(RS_FILL_WIRE | RS_CULL_BACK | RS_SCISSOR_ENABLE);

    float ratio;
    uint32_t w, h;
    swapchain->GetSize(&w, &h, &ratio);
    renderer->SetViewport((float)w, (float)h, 0, 0, 0.01f, 1.0f);
    renderer->SetScissor(0, 0, w, h);
    TextureDesc td;
    td.format = PF_DEPTHSTENCIL;
    td.width = w;
    td.height = h;
    td.sample_count = 1;
    td.usage = TU_DEPTHSTENCIL;
    auto depth = env->NewTexture(td);
    depth->CreateTexture(td);
    renderer->SetRenderTargets({rt}, depth);

    auto perspective = math::PerspectiveLH(math::PI / 4, ratio, 0.01f, 1000.0f);

    auto cube = model::Cube(2.f, 2.f, 2.f);
    auto cylinder = model::Cylinder(1.5f, 0.f, 2.f, 10, 30);
    auto ball = model::Ball(1.f, 20, 40);

    auto s = model::Scene(env);
    model::Scene *s1 = &s;
    model::Camera camera;
    s1->AddShape(&cube, "cube");
    s1->AddShape(&ball, "ball");
    s1->AddShape(&cylinder, "cylinder");

    while (app->LoopState() != WINDOW_STATE_HALT) {
      float color[4]{0.f, .5f, .5f, 1.0f};
      renderer->ClearRenderTarget(rt, color);
      renderer->ClearDepthStencil(depth);
      static float t = 0.1f;
      t += 0.01f;
      camera.SetPosition(0, 1.f, -8.f);
      camera.LookAt({0, 0, 1.f});
      // auto p = pose_.load();
      // camera.SetRotation(Quaternion(p.quat.w, -p.quat.x, -p.quat.y,
      // p.quat.z)); camera.Rotate(Quaternion(0, 0, 0.01f));
      auto view = camera.GetProjectionMatrix();
      auto mvp = view * perspective;
      renderer->SetModelViewPerspectiveProjection((float *)&mvp);

      cube.SetPosition(0.0f, -1.f, -1.f);
      cube.Rotate(Quaternion(0, 0.01f, 0));
      ball.Rotate(Quaternion(0, 0.01f, 0));
      cylinder.SetPosition(0, 1, 1);
      cylinder.SetRotation(Quaternion(sinf(t), cosf(t), 0));

      s1->Draw(renderer);
      renderer->SetBarrier();
      renderer->SyncBarrier();
      swapchain->Present();
    }
  } catch (const ll::engine::errors::Exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
  Env::CleanupEnvironment(env);
}
