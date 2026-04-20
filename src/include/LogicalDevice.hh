#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "vk_mem_alloc.h"

class PhysicalDevice;
class VulkanInstance;
struct GLFWwindow;

class LogicalDevice {
public:
	LogicalDevice();
	
	void findLogicaldevice(PhysicalDevice& physicalDevice, VulkanInstance& instance);
	void createSurface(VulkanInstance& Instance, GLFWwindow* window);
	vk::raii::SurfaceKHR& getSurface() { return surface; }
	vk::raii::Device& getLogicalDevice() { return m_logicalDevice; }
	uint32_t const getQueueIndex() const  { return queueIndex; }
	vk::raii::Queue GetQueue() { return m_graphicsQueue; }
	VmaAllocator GetAllocator() { return m_allocator; }
	~LogicalDevice();

private:
	uint32_t queueIndex = ~0;
	vk::raii::Device m_logicalDevice = nullptr;
	vk::raii::Queue m_graphicsQueue = nullptr;
	vk::raii::SurfaceKHR surface = nullptr;
	VmaAllocator m_allocator = nullptr;

	std::vector<const char*> requiredDeviceExtension = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName,
		vk::KHRShaderFloatControlsExtensionName,
		vk::KHRDynamicRenderingExtensionName 
	};
};


