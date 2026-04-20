#include "swapchain.hh"

#include <algorithm>

Swapchain::Swapchain()
{
}

Swapchain::~Swapchain()
{
	LOG("Swapchain Destructor");
}

void Swapchain::createSwapChain(PhysicalDevice& physicaldev ,LogicalDevice& device,GLFWwindow& window)
{   
	auto const& physicaldevice = physicaldev.device();
	auto const& surface = device.getSurface();
	
	auto surfaceCapabilities = physicaldevice.getSurfaceCapabilitiesKHR(*surface);
	swapChainExtent = chooseSwapExtent(surfaceCapabilities,window);
	swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicaldevice.getSurfaceFormatsKHR(*surface));
	vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.surface = *surface;
	
	swapChainCreateInfo.minImageCount = chooseSwapMinImageCount(surfaceCapabilities);
	swapChainCreateInfo.imageFormat = swapChainSurfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapChainExtent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapChainCreateInfo.presentMode = chooseSwapPresentMode(physicaldevice.getSurfacePresentModesKHR(*surface));
	swapChainCreateInfo.clipped = VK_TRUE;



	swapChain = vk::raii::SwapchainKHR(device.getLogicalDevice(), swapChainCreateInfo);
	swapChainImages = swapChain.getImages();


}

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,GLFWwindow& window)
{   
	if (capabilities.currentExtent.width != 0xFFFFFFFF)
	{
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(&window, &width, &height);

	return {
		 std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		 std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};



}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{   
	assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {return presentMode == vk::PresentModeKHR::eFifo; }));


	return std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR value) {return vk::PresentModeKHR::eMailbox == value; })
		? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{   
	assert(!availableFormats.empty());
	const auto formatIt = std::ranges::find_if(
		availableFormats,
		[](const auto& format) {
			return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

uint32_t Swapchain::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{   
	auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
	if ((0 < surfaceCapabilities.maxImageCount) && (minImageCount > surfaceCapabilities.maxImageCount))
	{
		minImageCount = surfaceCapabilities.maxImageCount;
	}


	return minImageCount;
}


vk::raii::ImageView Swapchain::createImageView(LogicalDevice& device, vk::Image image, vk::Format format)
{
	if (!image) {
		throw std::runtime_error("Invalid image");
	}

	vk::ImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
   return vk::raii::ImageView(device.getLogicalDevice(), imageViewCreateInfo);
}

void Swapchain::createImageViews(LogicalDevice& device)
{
	swapChainImageViews.clear();

	vk::ImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
	imageViewCreateInfo.format = swapChainSurfaceFormat.format;

	imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	for (const auto& image : swapChainImages)
	{
		imageViewCreateInfo.image = image;
		swapChainImageViews.emplace_back(
			vk::raii::ImageView(device.getLogicalDevice(), imageViewCreateInfo)
		);
	}
}

void Swapchain::createTextureImageView(LogicalDevice& device,TextureVK& texture)
{
	
	textureImageView = createImageView(device, *texture.getTextureImage(), vk::Format::eR8G8B8A8Srgb);
}


void Swapchain::createTextureSampler(LogicalDevice& device, PhysicalDevice& physicaldevice)
{
	vk::PhysicalDeviceProperties properties = physicaldevice.device().getProperties();
	
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	textureSampler = vk::raii::Sampler(device.getLogicalDevice(), samplerInfo);
}

void Swapchain::cleanupSwapChain()
{
	swapChainImageViews.clear();
	swapChain = nullptr;
}



