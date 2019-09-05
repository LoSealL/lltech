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

TEST(engine, show_image) {
  auto env = Env::NewEnvironment(GRAPHIC_API_DX11);
  Env::Advanced::SetLogLevel(0);
  WindowDesc desc{};
  desc.name = "TestImshow";
  desc.style = WINDOW_STYLE_DEFAULT;
  desc.width = 1280;
  desc.height = 720;
  auto app = env->NewWindow(desc);
  app->Show();
  app->SetPos(1920 / 4, 1200 / 4);

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
  renderer->SetRasterizer(RS_FILL_SOLID | RS_CULL_NONE | RS_SCISSOR_ENABLE);
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
  renderer->SetSampler();
  td.format = PF_RGBA32_UNORM;
  td.usage = TU_SHADERRESOURCE;
  auto res = env->NewTexture(td);

  auto perspective = math::PerspectiveLH(math::PI / 4, ratio, 0.01f, 1000.0f);

  auto canvas = model::Plain(1.f, 1.f);
  auto s = model::Scene(env);
  model::Scene *s1 = &s;
  model::Camera camera;
  s1->AddShape(&canvas, "ball");

  while (app->LoopState() != WINDOW_STATE_HALT) {
    float color[4]{0.f, .5f, .5f, 1.0f};
    renderer->ClearRenderTarget(rt, color);
    renderer->ClearDepthStencil(depth);
    renderer->SetShaderResources(res);
    camera.SetPosition(0, 0.f, -1.f);
    camera.LookAt({0, 0, 0.f});
    auto view = camera.GetProjectionMatrix();
    auto mvp = view * perspective;
    renderer->SetModelViewPerspectiveProjection((float *)&mvp);
    canvas.SetPosition(0, 0, 0);
    canvas.Rotate(Quaternion(0, 0.01f, 0));
    s1->Draw(renderer);
    swapchain->Present();
  }
  Env::CleanupEnvironment(env);
}
