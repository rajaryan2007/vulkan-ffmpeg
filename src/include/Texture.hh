#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include "vk_mem_alloc.h"

class UBObuffer;
class LogicalDevice;
class CommandPool;
class TextureVK {
public:
	TextureVK() = default;
	~TextureVK();

	void CreateTextureVK(UBObuffer& Ubobuffer, LogicalDevice& logicaldev, CommandPool& commandPool, uint32_t width, uint32_t height);

	// create texture sized for video (no file loading)
	void CreateTextureForVideo(UBObuffer& Ubobuffer, LogicalDevice& logicaldev, CommandPool& commandPool, uint32_t width, uint32_t height);

	// upload new RGBA pixel data to the existing texture (called every frame for video)
	void updateTexture(const uint8_t* pixels, uint32_t width, uint32_t height, LogicalDevice& logicaldev, CommandPool& commandPool);
	void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, VmaMemoryUsage memUsage, vk::ImageTiling tiling,vk::raii::Image& image, VmaAllocation& imageMemory,LogicalDevice& logicaldev);
	const vk::raii::Image& getTextureImage() const { return textureImage; }
	const vk::raii::DeviceMemory& getTextureImageMemory() const { return textureImageMemory; }
	
	void setAllocator(VmaAllocator allocator) { m_allocator = allocator; }
private:
	vk::raii::Image textureImage = nullptr;
	vk::raii::DeviceMemory textureImageMemory = nullptr;
	VmaAllocation textureImageMemoryAlloc = nullptr;
	vk::raii::Buffer stagingBuffer = nullptr;
	VmaAllocation stagingBufferMemory = nullptr;
	VmaAllocator m_allocator = nullptr;
};