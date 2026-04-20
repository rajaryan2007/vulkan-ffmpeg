#include "Logger.h"

#define NOMINMAX
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <vector>
#define GLFW_INCLUDE_VULKAN        // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>
#include "physicalDevice.hh"
#include "LogicalDevice.hh"
#include "Texture.hh"






class Swapchain {
public:
	Swapchain();
	~Swapchain();
	void createSwapChain(PhysicalDevice& physicaldev, LogicalDevice& device,GLFWwindow& window);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,GLFWwindow& window);
	static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
	static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
	void createImageViews(LogicalDevice& device);
	void cleanupSwapChain();
	
	vk::raii::ImageView createImageView(LogicalDevice& device, vk::Image image, vk::Format format);
	void createTextureImageView(LogicalDevice& device, TextureVK& texture);
	void createTextureSampler(LogicalDevice& device, PhysicalDevice& physicaldevice);

	const vk::Extent2D& GetExtent() const { return swapChainExtent; }
	const vk::SurfaceFormatKHR GetSurfaceFormat() const { return swapChainSurfaceFormat; };
	const std::vector<vk::Image>& GetImage() const { return swapChainImages; }
	const vk::raii::SwapchainKHR& getSwapChain() { return swapChain; }
	const std::vector<vk::raii::ImageView>& GetImageView() const { return swapChainImageViews; }
	const vk::raii::ImageView& getTextureImageView() const { return textureImageView; }
	const vk::raii::Sampler& getTextureSampler() const { return textureSampler; }
private:
	vk::raii::SwapchainKHR           swapChain = nullptr;
	std::vector<vk::Image>           swapChainImages;
	vk::SurfaceFormatKHR             swapChainSurfaceFormat;
	vk::Extent2D                     swapChainExtent;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	vk::raii::Sampler      textureSampler = nullptr;

	vk::raii::ImageView    textureImageView = nullptr;
};




