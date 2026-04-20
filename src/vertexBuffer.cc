#include "vertexBuffer.hh"
#include "LogicalDevice.hh"
#include "physicalDevice.hh"
#include "utils.hh"
#include <stdexcept>
#include <cstring>



VertexBuffer::VertexBuffer()
	: vertexBuffer(nullptr), vertexBufferAllocation(nullptr), vertexBufferSize(0)
{

}

VertexBuffer::~VertexBuffer()
{
	if (m_allocator) {
		if (vertexBuffer != nullptr) {
			vmaDestroyBuffer(m_allocator, *vertexBuffer, vertexBufferAllocation);
			vertexBuffer.release();
		}
		if (indexBuffer != nullptr) {
			vmaDestroyBuffer(m_allocator, *indexBuffer, indexBufferAllocation);
			indexBuffer.release();
		}
	}
}

uint32_t VertexBuffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, PhysicalDevice& physicalDev)
{
	vk::PhysicalDeviceMemoryProperties memProperties = physicalDev.device().getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VertexBuffer::createVertexBuffer(const vk::raii::CommandPool& commandPool, PhysicalDevice& physicalDev, LogicalDevice& logicalDev, const std::vector<Vertex>& vertices)
{
	m_allocator = logicalDev.GetAllocator();
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	
	vk::raii::Buffer stagingBuffer(nullptr);
	VmaAllocation stagingAllocation;
	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAllocation, logicalDev);

	void* data;
	vmaMapMemory(logicalDev.GetAllocator(), stagingAllocation, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(logicalDev.GetAllocator(), stagingAllocation);

	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexBufferAllocation, logicalDev);

	copyBuffer(commandPool, logicalDev, stagingBuffer, vertexBuffer, bufferSize);
	
	// Clean up staging buffer
	// Note: In a real app, you might want to wait for the transfer to finish before destroying staging resources
	// but here copyBuffer calls graphicsQueue.waitIdle().
	vmaDestroyBuffer(logicalDev.GetAllocator(), *stagingBuffer, stagingAllocation);
	// We need to release the RAII handle so it doesn't try to vkDestroyBuffer twice
	stagingBuffer.release();
}




void VertexBuffer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, vk::raii::Buffer& buffer, VmaAllocation& allocation, LogicalDevice& LogDevice)
{
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = (VkBufferUsageFlags)usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = memoryUsage;

	VkBuffer vkBuffer;
	if (vmaCreateBuffer(LogDevice.GetAllocator(), &bufferInfo, &allocInfo, &vkBuffer, &allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer via VMA!");
	}

	buffer = vk::raii::Buffer(LogDevice.getLogicalDevice(), vkBuffer);
}

void VertexBuffer::copyBuffer(const vk::raii::CommandPool& commandPool, LogicalDevice& device, vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{

	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;

	vk::raii::CommandBuffer commandCopyBuffer = std::move(device.getLogicalDevice().allocateCommandBuffers(allocInfo).front());

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandCopyBuffer.begin(beginInfo);
	commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));

	commandCopyBuffer.end();

	const auto& graphicsQueue = device.GetQueue();
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*commandCopyBuffer;

	graphicsQueue.submit(submitInfo, nullptr);
	graphicsQueue.waitIdle();
};

void VertexBuffer::createIndexBuffer(const vk::raii::CommandPool& commandPool, PhysicalDevice& physicalDev, LogicalDevice& logicalDev, const std::vector<uint16_t>& indices)
{
	m_indices = indices;
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	vk::raii::Buffer stagingBuffer(nullptr);
	VmaAllocation stagingAllocation;
	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAllocation, logicalDev);

	void* data;
	vmaMapMemory(logicalDev.GetAllocator(), stagingAllocation, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vmaUnmapMemory(logicalDev.GetAllocator(), stagingAllocation);

	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_ONLY, indexBuffer, indexBufferAllocation, logicalDev);

	copyBuffer(commandPool, logicalDev, stagingBuffer, indexBuffer, bufferSize);
	
	vmaDestroyBuffer(logicalDev.GetAllocator(), *stagingBuffer, stagingAllocation);
	stagingBuffer.release();
}

