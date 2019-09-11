/********************************************************************
Copyright 2018 Tang, Wenyi. All Rights Reserved.
Description : HLSL Shader
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 28th, 2018
********************************************************************/
#ifndef LL_GRAPHIC_CORE_SHADER_SHADER_HLSL_H_
#define LL_GRAPHIC_CORE_SHADER_SHADER_HLSL_H_
#include <d3dcompiler.h>
#include "ll_graphic/core/shader/shader.h"

namespace ll {
namespace engine {
namespace core {
/* Compile, generate HLSL shader */
class ShaderHLSL : public Shader {
 public:
  ShaderHLSL();
  ~ShaderHLSL();

  virtual void Compile(ShaderCompileTarget target) override;

  virtual void Compile(String filename, ShaderCompileTarget target) override;

  virtual void Compile(const char *src, size_t size,
                       ShaderCompileTarget target) override;

  virtual void GetCodeBytes(void **bytes, size_t *size) override;

  virtual void SetEntryPoint(const char *entry) override;

  virtual void SetFlag(const uint32_t flag) override;

  virtual ShaderCompileTarget GetShaderTarget() const override;

 private:
  String entry_;
  uint32_t target_;
  uint32_t flag_;
  bool compiled_;
  ID3DBlob *blob_;
};
}  // namespace core
}  // namespace engine
}  // namespace ixr

#endif  // LL_GRAPHIC_CORE_SHADER_SHADER_HLSL_H_
