
#include "Texture.hh"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Logger.h"
#include "index_buffer.hh"
#include "LogicalDevice.hh"
#include "CommandPool.hh"
TextureVK::~TextureVK()
{
	LOG("Texture Get construted");
	if (m_allocator) {
		if (stagingBuffer != nullptr && stagingBufferMemory != nullptr) {
			vmaDestroyBuffer(m_allocator, *stagingBuffer, stagingBufferMemory);
			stagingBuffer.release();
		}
		if (textureImage != nullptr && textureImageMemoryAlloc != nullptr) {
			vmaDestroyImage(m_allocator, *textureImage, textureImageMemoryAlloc);
			textureImage.release();
		}
	}
}

void TextureVK::CreateTextureVK(UBObuffer& Ubobuffer, LogicalDevice& logicalDev, CommandPool& commandPool, uint32_t width, uint32_t height)
{
	m_allocator = logicalDev.GetAllocator();
	
	int TexWidth, texHeight, texChannels;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc* pixel = stbi_load("C:\\Users\\rion\\Downloads\\CAT.jpg", &TexWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	vk::DeviceSize ImageSize = TexWidth * texHeight * 4;


	if (!pixel)
	{
		throw std::runtime_error("failed to load texture image");
	}

	Ubobuffer.createBuffer(ImageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingBufferMemory, logicalDev);
	
	void* data;
	vmaMapMemory(logicalDev.GetAllocator(), stagingBufferMemory, &data);
	memcpy(data, pixel, ImageSize);
	vmaUnmapMemory(logicalDev.GetAllocator(), stagingBufferMemory);
	stbi_image_free(pixel);

	// create the texture image
	CreateImage(TexWidth, texHeight, vk::Format::eR8G8B8A8Srgb, 
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		VMA_MEMORY_USAGE_GPU_ONLY, vk::ImageTiling::eOptimal,
		textureImage, textureImageMemoryAlloc, logicalDev);


    commandPool.transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, logicalDev);
    commandPool.copyBufferToImage(stagingBuffer, textureImage, TexWidth, texHeight, logicalDev);
    commandPool.transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, logicalDev);
}



void TextureVK::CreateImage(uint32_t width, uint32_t height, vk::Format format,
	vk::ImageUsageFlags usage, VmaMemoryUsage memUsage, vk::ImageTiling tiling,
	vk::raii::Image& image, VmaAllocation& imageMemory, LogicalDevice& logicaldev)
{
	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent = vk::Extent3D{ width, height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;

	
	VkImageCreateInfo rawImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memUsage;

	
	VkImage rawImage;
	if (vmaCreateImage(logicaldev.GetAllocator(), &rawImageInfo, &allocInfo, &rawImage, &imageMemory, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image via VMA!");
	}

	
	image = vk::raii::Image(logicaldev.getLogicalDevice(), rawImage);
}

void TextureVK::CreateTextureForVideo(UBObuffer& Ubobuffer, LogicalDevice& logicalDev, CommandPool& commandPool, uint32_t width, uint32_t height)
{
	m_allocator = logicalDev.GetAllocator();

	vk::DeviceSize ImageSize = width * height * 4; 

	
	Ubobuffer.createBuffer(ImageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingBufferMemory, logicalDev);

	
	CreateImage(width, height, vk::Format::eR8G8B8A8Srgb,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		VMA_MEMORY_USAGE_GPU_ONLY, vk::ImageTiling::eOptimal,
		textureImage, textureImageMemoryAlloc, logicalDev);

	
	commandPool.transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, logicalDev);
}

void TextureVK::updateTexture(const uint8_t* pixels, uint32_t width, uint32_t height, LogicalDevice& logicalDev, CommandPool& commandPool)
{
	vk::DeviceSize ImageSize = width * height * 4;

	
	void* data;
	vmaMapMemory(logicalDev.GetAllocator(), stagingBufferMemory, &data);
	memcpy(data, pixels, ImageSize);
	vmaUnmapMemory(logicalDev.GetAllocator(), stagingBufferMemory);

	
	commandPool.transitionImageLayout(textureImage, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal, logicalDev);

	
	commandPool.copyBufferToImage(stagingBuffer, textureImage, width, height, logicalDev);

	
	commandPool.transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, logicalDev);
}
