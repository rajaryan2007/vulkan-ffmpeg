#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <vector>
#include <cstdint>
#include "volk.h"
#include "vk_mem_alloc.h"

struct Vertex;
class PhysicalDevice;
class LogicalDevice;

class VertexBuffer {
	public:
	VertexBuffer();
	~VertexBuffer();

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, PhysicalDevice& physicalDev);
	void createVertexBuffer(const vk::raii::CommandPool& commandPool,PhysicalDevice& physicalDev, LogicalDevice& logicalDev, const std::vector<Vertex>& vertices);
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, vk::raii::Buffer& buffer, VmaAllocation& allocation, LogicalDevice& LogDevice);
	vk::raii::Buffer& get()  { return vertexBuffer; }
	void copyBuffer(const vk::raii::CommandPool& commandPool, LogicalDevice& device,vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
	void createIndexBuffer(const vk::raii::CommandPool& commandPool, PhysicalDevice& physicalDev, LogicalDevice& logicalDev, const std::vector<uint16_t>& indices);
	const std::vector<uint16_t>& getIndices() const { return m_indices; }
	vk::raii::Buffer& getIndexBuffer() { return indexBuffer; }

private:
	std::vector<uint16_t> m_indices;

	vk::raii::Buffer vertexBuffer = nullptr;
	VmaAllocation vertexBufferAllocation = nullptr;

	vk::raii::Buffer indexBuffer = nullptr;
	VmaAllocation indexBufferAllocation = nullptr;

	VmaAllocator m_allocator = nullptr;
	uint32_t vertexBufferSize;
};

