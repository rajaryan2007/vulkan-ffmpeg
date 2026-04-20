#pragma once
#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <vector>
#include "instance.hh"

class PhysicalDevice {
public:
	PhysicalDevice();
	~PhysicalDevice();
	void createPhysicalDevice(VulkanInstance& myInstanceWrapper);

	inline vk::raii::PhysicalDevice& device() { return m_physical_device; }

private:
	vk::raii::PhysicalDevice m_physical_device = nullptr;
	
	std::vector<const char*> requiredDeviceExtension = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName,
		vk::KHRShaderFloatControlsExtensionName,
		vk::KHRDynamicRenderingExtensionName };
};

