#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <vector>
#include <cstdint>
#include "glm/glm.hpp"
#include "volk.h"
#include "vk_mem_alloc.h"

class LogicalDevice;
class PhysicalDevice;
class CommandPool;
class Swapchain;

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class UBObuffer {
public:
	void createDescriptorSetLayout(LogicalDevice& logicaldev, uint32_t MAX_FRAMES_IN_FLIGHT);

	void createUniformBuffers(CommandPool& m_commandPool, PhysicalDevice& physicalDev, LogicalDevice& logicalDev);
	
	void updateUniformBuffer(uint32_t currentImage);
	
	~UBObuffer();
	
	const vk::raii::DescriptorSetLayout& getDescriptorSet () const { return descriptorSetLayout; }

	const UniformBufferObject& getUBO() const { return ubo; }

	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, vk::raii::Buffer& buffer, VmaAllocation& allocation, LogicalDevice& LogicalDev);
	
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, PhysicalDevice physicalDevice);

	void createDescriptorPool(LogicalDevice& logicalDev, const CommandPool& commandPool, uint32_t MAX_FRAMES_IN_FLIGHT);

	void createDescriptorSets(LogicalDevice& logicalDev, const CommandPool& commandPool, uint32_t MAX_FRAMES_IN_FLIGHT, const Swapchain& swapchain);


private:
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
	vk::raii::PipelineLayout pipelineLayout = nullptr;

	vk::raii::Buffer indexBuffer = nullptr;
	VmaAllocation indexBufferAllocation = nullptr;

	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;

	UniformBufferObject ubo{};

	std::vector<vk::raii::Buffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBuffersAllocation;
	std::vector<void*> uniformBuffersMapped;
	
	VmaAllocator m_allocator = nullptr;
	
public:
	const std::vector<vk::raii::DescriptorSet>& getDescriptorSets() const { return descriptorSets; }



};


