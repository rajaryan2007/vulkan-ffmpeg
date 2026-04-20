#include "volk.h"
#include "LogicalDevice.hh"
#include "physicalDevice.hh"
#include "instance.hh"
#include <glfw/glfw3.h>
#include <algorithm>
#include <assert.h>
#include "Logger.h"

LogicalDevice::LogicalDevice()
    : m_logicalDevice(nullptr), m_graphicsQueue(nullptr), requiredDeviceExtension{
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName,
        vk::KHRShaderFloatControlsExtensionName,
        vk::KHRDynamicRenderingExtensionName }
{
   
}

void LogicalDevice::findLogicaldevice(PhysicalDevice& physicalDevice, VulkanInstance& Instance)
{
	auto const& physicalDev = physicalDevice.device();
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDev.getQueueFamilyProperties();

	
	for (uint32_t qfqIndex = 0; qfqIndex < queueFamilyProperties.size(); qfqIndex++) {
		if ((queueFamilyProperties[qfqIndex].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDev.getSurfaceSupportKHR(qfqIndex, *surface))
		{
			queueIndex = qfqIndex;
			break;
		}
	}
	if (queueIndex == ~0) {
		throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
	}
	vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain;

	// 1. Get references to the specific feature structs in the chain
	auto& features2 = featureChain.get<vk::PhysicalDeviceFeatures2>();
	auto& features13 = featureChain.get<vk::PhysicalDeviceVulkan13Features>();
	auto& featuresExt = featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	// 2. Set base Vulkan 1.0 features (nested inside PhysicalDeviceFeatures2)
	features2.features.samplerAnisotropy = VK_TRUE;

	// 3. Set Vulkan 1.3 features
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;

	// 4. Set Extension features
	featuresExt.extendedDynamicState = VK_TRUE;

	float queuePriority = 0.5f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
	deviceQueueCreateInfo.queueFamilyIndex = queueIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

	vk::DeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo,
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	m_logicalDevice = vk::raii::Device(physicalDev, deviceCreateInfo);
	volkLoadDevice(*m_logicalDevice);
	m_graphicsQueue = vk::raii::Queue(m_logicalDevice, queueIndex, 0);

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = *physicalDev;
	allocatorInfo.device = *m_logicalDevice;
	allocatorInfo.instance = *Instance.instance();
	allocatorInfo.pVulkanFunctions = &vulkanFunctions;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

	if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
		throw std::runtime_error("failed to create VMA allocator!");
	}
}

void LogicalDevice::createSurface(VulkanInstance& Instance, GLFWwindow* window)
{
	auto const&  instance = Instance.instance();
 
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
		{
			throw std::runtime_error("failed to create window surface!");
		}
		surface = vk::raii::SurfaceKHR(instance, _surface);
	
}



LogicalDevice::~LogicalDevice()
{
	LOG("LogicalDevice Destructor");
	if (m_allocator != nullptr) {
		vmaDestroyAllocator(m_allocator);
	}
}
