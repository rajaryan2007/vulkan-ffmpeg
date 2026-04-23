#include "CommandPool.hh"
#include "LogicalDevice.hh"
#include "swapchain.hh"
#include "GrapicPipeline.hh"
#include "utils.hh"

void CommandPool::Init(LogicalDevice& Device)
{
	const auto& queueIndex = Device.getQueueIndex();
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queueIndex;
	

	
	m_commandPool.emplace(Device.getLogicalDevice(), poolInfo);
}

void CommandPool::createCommandBuffer(LogicalDevice& Device)
{
		if (!m_commandPool.has_value()) {
		throw std::runtime_error("CommandPool used before Init was called!");
	}

	vk::CommandBufferAllocateInfo allocInfo{};
	
	allocInfo.commandPool = *m_commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	
	m_commandBuffer = vk::raii::CommandBuffers(Device.getLogicalDevice(), allocInfo);
}

void CommandPool::transition_image_layout(auto& cmd,Swapchain& swapchain,uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask)
{   
	
	auto& swapChainImages = swapchain.GetImage();
	vk::ImageMemoryBarrier2 barrier = {};
	barrier.srcStageMask = src_stage_mask;
	barrier.srcAccessMask = src_access_mask;
	barrier.dstStageMask = dst_stage_mask;
	barrier.dstAccessMask = dst_access_mask;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = swapChainImages[imageIndex];
	barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor,0,1,0,1 };
    
	vk::DependencyInfo dependency_info{};
	dependency_info.dependencyFlags = {};
	dependency_info.imageMemoryBarrierCount = 1;
	dependency_info.pImageMemoryBarriers = &barrier;

	cmd.pipelineBarrier2(dependency_info);

}


void CommandPool::transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, LogicalDevice& LogicalDev)
{
	std::unique_ptr<vk::raii::CommandBuffer> CommandBuffer = beginSingleTimeCommands(LogicalDev);

	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image;
	barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor,0,1,0,1 };

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		// For video: transition back from shader read to accept new frame data
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::invalid_argument("unsuppported layout transition");
	}
	CommandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
	endSingleTimeCommand(*CommandBuffer,LogicalDev);

}

void CommandPool::recordCommandBuffer(vk::raii::Buffer& vertexBuffer,GrapicPileline& grapic,uint32_t imageIndex, Swapchain& swapchian,const vk::raii::Buffer& IndexBuffer, const std::vector<uint16_t>& indices, const std::vector<vk::raii::DescriptorSet>& descriptorSets)
{ 
	const auto& pipelineLayout = grapic.GetPipelineLayout();
	auto& cmd = m_commandBuffer[frameIndex];
    

	cmd.begin({});


	transition_image_layout(
		cmd,
		swapchian,
		imageIndex,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal,
		{},
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput
	);

	
    vk::ClearValue  clearColor = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    
	const auto& swapChainImageView= swapchian.GetImageView();
	vk::RenderingAttachmentInfo attachmentInfo{};
	attachmentInfo.imageView = swapChainImageView[imageIndex];
	attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentInfo.clearValue = clearColor;
	


	vk::RenderingInfo renderingInfo{};
	VkOffset2D offset = { 0, 0 };
	VkExtent2D extent = swapchian.GetExtent();

	renderingInfo.renderArea.offset = offset;
	renderingInfo.renderArea.extent = extent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &attachmentInfo;

	cmd.beginRendering(renderingInfo);
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, grapic.GetPipeline());
	cmd.setViewport(0, vk::Viewport(0, 0, (float)extent.width, (float)extent.height, 0, 1));
	cmd.setScissor(0, vk::Rect2D({ 0,0 }, extent));
	
	

	cmd.bindVertexBuffers(
		0,
		*vertexBuffer,
		std::array<vk::DeviceSize, 1>{ 0 }
	);
	cmd.bindIndexBuffer(
		*IndexBuffer,
		0,
		vk::IndexType::eUint16
	);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr);
	cmd.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	cmd.endRendering();

	

	transition_image_layout(
		cmd,
		swapchian,
		imageIndex,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		{},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eNone
	);

	cmd.end();
}

void CommandPool::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height,LogicalDevice& logicalDev)
{
	std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands(logicalDev);
	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource = { vk::ImageAspectFlagBits::eColor,0,0,1 };
	region.imageOffset = vk::Offset3D{ 0,0,0 };
	region.imageExtent = vk::Extent3D{ width,height,1 };

	commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });

	endSingleTimeCommand(*commandBuffer,logicalDev);
}

std::unique_ptr<vk::raii::CommandBuffer> CommandPool::beginSingleTimeCommands(LogicalDevice& logicalDev)
{
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = *m_commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;

	std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(std::move(vk::raii::CommandBuffers(logicalDev.getLogicalDevice(), allocInfo).front()));

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer->begin(beginInfo);

	return commandBuffer;
}

void CommandPool::endSingleTimeCommand(vk::raii::CommandBuffer& commandBuffer,LogicalDevice& logicalDev)
{
	commandBuffer.end();
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*commandBuffer;

	auto m_graphicQueue = logicalDev.GetQueue();
	m_graphicQueue.submit(submitInfo, nullptr);
	m_graphicQueue.waitIdle();
}


