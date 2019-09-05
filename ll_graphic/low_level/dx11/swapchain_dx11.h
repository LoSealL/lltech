/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : swapchain interface
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Mar. 6th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_LOW_LEVEL_DX11_SWAPCHAIN_DX11_H_
#define LL_GRAPHIC_LOW_LEVEL_DX11_SWAPCHAIN_DX11_H_
#include "ll_graphic/core/swapchain/swapchain.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace ll {
namespace engine {
class Env;

namespace dx11 {
class Dx11SwapChain : public core::SwapChain {
public:
  template <class T> using CPtr = Microsoft::WRL::ComPtr<T>;

  Dx11SwapChain(Env *e);
  
  virtual ~Dx11SwapChain();

  virtual void CreateSwapChain(const SwapChainDesc &sd) override;

  virtual core::Texture *GetBuffer(uint32_t index) override;

  virtual void Present(int vsync) override;

  virtual void Resize(uint32_t w, uint32_t h) override;

  virtual void GetSize(uint32_t *w, uint32_t *h, float *ratio) override;

private:
  Env *env_;
  CPtr<IDXGISwapChain1> swapchain_;
};
} // namespace dx11
} // namespace engine
} // namespace ll
#endif // LL_GRAPHIC_LOW_LEVEL_DX11_SWAPCHAIN_DX11_H_
