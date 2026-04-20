#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <optional>
#include <vector>


class LogicalDevice;
class Swapchain;
class GrapicPileline;
class CommandPool {
public:
	CommandPool() = default;
	void Init(LogicalDevice& Device);
	void createCommandBuffer(LogicalDevice& Device);
	static void transition_image_layout(
		auto& cmd,
		Swapchain& swapchian,
		uint32_t                imageIndex,
		vk::ImageLayout         old_layout,
		vk::ImageLayout         new_layout,
		vk::AccessFlags2        src_access_mask,
		vk::AccessFlags2        dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask);
	 void recordCommandBuffer(vk::raii::Buffer& vertexBuffer, GrapicPileline& grapic, uint32_t imageIndex, Swapchain& swapchian, const vk::raii::Buffer& IndexBuffer, const std::vector<uint16_t>& indices, const std::vector<vk::raii::DescriptorSet>& descriptorSets);
	const vk::raii::CommandBuffer&  GetCommandBuffer() const {return m_commandBuffer[frameIndex];}
	constexpr int GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
    uint32_t& GetFrameIndex() { return frameIndex; }
	const vk::raii::CommandPool& GetCommandPool() const { return *m_commandPool; }

	void copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height,LogicalDevice& logicalDev);

	std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands(LogicalDevice& logicaldev);
	void endSingleTimeCommand(vk::raii::CommandBuffer& commandBuffer, LogicalDevice& logicalDev);

	void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, LogicalDevice& LogicalDev);

private:  
	
	uint32_t frameIndex = 0;
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	std::optional<vk::raii::CommandPool> m_commandPool;

    std::vector<vk::raii::CommandBuffer>  m_commandBuffer;
};

