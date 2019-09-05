/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : format mapper
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_LOW_LEVEL_DX11_MAPPER_DX11_H_
#define LL_GRAPHIC_LOW_LEVEL_DX11_MAPPER_DX11_H_
#include <d3d11.h>
#include "ll_graphic/core/gpu_buffer/gpu_buffer.h"
#include "ll_graphic/core/texture/texture.h"
#include "ll_graphic/core/vertex/vertex_element.h"
#include "ll_graphic/engine/types.h"

namespace ll {
namespace engine {
namespace dx11 {

DXGI_FORMAT ToDxgiFormat(PixelFormat fmt);

DXGI_FORMAT ToDxgiFormat(VertexElementType type);

D3D11_USAGE ToUsage(TextureUsage usage);

D3D11_USAGE ToUsage(BufferUsage usage);

D3D11_BIND_FLAG ToBindFlag(TextureUsage usage);

D3D11_BIND_FLAG ToBindFlag(BufferUsage usage);

D3D11_CPU_ACCESS_FLAG ToCpuAccess(TextureUsage usage);

D3D11_CPU_ACCESS_FLAG ToCpuAccess(BufferUsage usage);

D3D11_RASTERIZER_DESC ToRasterizerDesc(RasterizerDesc rd);

D3D11_SAMPLER_DESC ToSamplerDesc();
}  // namespace dx11
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_LOW_LEVEL_DX11_MAPPER_DX11_H_
