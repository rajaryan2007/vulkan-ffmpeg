#include "volk.h"
#include "instance.hh"
#include <glfw/glfw3.h>

const std::vector<char const*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif
#include "Logger.h"



VulkanInstance::VulkanInstance()
	: m_instance(nullptr), m_context()
{
	if (volkInitialize() != VK_SUCCESS) {
		throw std::runtime_error("failed to initialize volk!");
	}
}

void VulkanInstance::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vk_engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	std::vector<const char*> requiredLayers;
	if (enableValidationLayers) {
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
	}
	auto layerProperties = m_context.enumerateInstanceLayerProperties();
	if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
		return std::ranges::none_of(layerProperties,
			[requiredLayer](auto const& layerProperty)
			{ return strcmp(layerProperty.layerName, requiredLayer) == 0; });
		}))
	{
		throw std::runtime_error("One or more required layers are not supported!");
	}

	auto requiredExtensions = getRequiredExtensions();


	auto extensionProperties = m_context.enumerateInstanceExtensionProperties();
	for (auto const& requiredExtension : requiredExtensions) {
		if (std::ranges::none_of(extensionProperties, [&requiredExtension](auto const& extensionProperty) {
			return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
			})) {
			throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
		}
	}


	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		debugCreateInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

		debugCreateInfo.messageType =
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

		debugCreateInfo.pfnUserCallback = debugCallback;
	}


	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
	createInfo.ppEnabledLayerNames = requiredLayers.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.empty() ? nullptr : requiredExtensions.data();;
	if (enableValidationLayers) {
		createInfo.pNext = &debugCreateInfo;
	}

	m_instance = vk::raii::Instance(m_context, createInfo);
	volkLoadInstance(*m_instance);
	if (enableValidationLayers) {
		m_debugMessenger =
			vk::raii::DebugUtilsMessengerEXT(m_instance, debugCreateInfo);
	}
}

VulkanInstance::~VulkanInstance()
{
	LOG("VulkanInstance Destructor");
}


vk::raii::Instance& VulkanInstance::instance() 
{
	return m_instance;
}



std::vector<const char*> VulkanInstance::getRequiredExtensions()
{   
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanInstance::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{   
	if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		std::cerr << "validation layer: type"
			<< to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
	}


	return vk::False;
}




