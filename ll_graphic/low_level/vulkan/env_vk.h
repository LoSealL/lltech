/********************************************************************
Copyright 2018 Intel Corp. All Rights Reserved.
Description : vulkan implement
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : April 2nd, 2018
********************************************************************/
#ifndef LL_GRAPHIC_LOW_LEVEL_VULKAN_ENV_VK_H_
#define LL_GRAPHIC_LOW_LEVEL_VULKAN_ENV_VK_H_
#include <vulkan/vulkan.h>
namespace ll {
namespace engine {
namespace vk {
struct VulkanEnvironment {
  VkInstance ins;
  VkPhysicalDevice phy_dev;
  VkDevice dev;
  VkQueue queue[4];
};
}  // namespace vk
}  // namespace engine
}  // namespace ixr
#endif  // LL_GRAPHIC_LOW_LEVEL_VULKAN_ENV_VK_H_
