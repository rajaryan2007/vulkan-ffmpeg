#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <glm/glm.hpp>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include "LogicalDevice.hh"

static inline std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	std::vector<char> buffer(file.tellg());

	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

	file.close();
	return buffer;
}

struct vkCoreutils {
	vk::raii::Device device;
	vk::raii::PhysicalDevice physicalDevice;
	vk::raii::CommandPool commandPool;
	vk::raii::DescriptorPool descriptorPool;
	vk::raii::DescriptorSetLayout descriptorSetLayout;
	vk::raii::PipelineLayout pipelineLayout;
	vk::raii::Pipeline graphicsPipeline;
	vk::raii::Instance instance;
	vk::raii::SurfaceKHR surface;

	void init(LogicalDevice& logical, vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface, vk::raii::PhysicalDevice& physicalDevice, vk::raii::Device& device, vk::raii::CommandPool& commandPool, vk::raii::DescriptorPool& descriptorPool, vk::raii::DescriptorSetLayout& descriptorSetLayout, vk::raii::PipelineLayout& pipelineLayout, vk::raii::Pipeline& graphicsPipeline)
	{
		this->instance = std::move(instance);
		this->surface = std::move(surface);
		this->physicalDevice = std::move(physicalDevice);
		this->device = std::move(device);
		this->commandPool = std::move(commandPool);
		this->descriptorPool = std::move(descriptorPool);
		this->descriptorSetLayout = std::move(descriptorSetLayout);
		this->pipelineLayout = std::move(pipelineLayout);
		this->graphicsPipeline = std::move(graphicsPipeline);
	};
};


struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription() {
		return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
		};
	}
};





[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code,LogicalDevice& logical);