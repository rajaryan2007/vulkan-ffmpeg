

#include "GrapicPipeline.hh"
#include "Logger.h"
#include "utils.hh"
#include "index_buffer.hh"



GrapicPileline::GrapicPileline()
{
	LOG("GrapicPipeline create")
}

GrapicPileline::~GrapicPileline()
{
	LOG("GrapicPipeline destroy")
}



void GrapicPileline::Init(LogicalDevice& Device, vk::Extent2D swapChainExtent, UBObuffer& ubodesptorset,vk::SurfaceFormatKHR swapChainSurfaceFormat)
{
	LOG("GrapicPipeline Init")
		const auto& m_descriptorsetLayoutInfo = ubodesptorset.getDescriptorSet();

	vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shader/slang.spv"),Device);
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    
	auto bindingDescription    = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();


	vk::PipelineVertexInputStateCreateInfo   vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	std::vector dynamicStates = {
	   vk::DynamicState::eViewport,
	   vk::DynamicState::eScissor
	};
     
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();


	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	
	vk::Viewport{ 0.0f,0.0f,static_cast<float>(swapChainExtent.width),static_cast<float>(swapChainExtent.height),0.0f,1.0f };

	vk::PipelineViewportStateCreateInfo      viewportState{};
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;


	vk::PipelineRasterizationStateCreateInfo rasterizer(
		{},                         // flags
		vk::False,                  // depthClampEnable
		vk::False,                  // rasterizerDiscardEnable
		vk::PolygonMode::eFill,     // polygonMode
		vk::CullModeFlagBits::eBack,// cullMode
		vk::FrontFace::eCounterClockwise,  // frontFace
		vk::False,                  // depthBiasEnable
		0.0f,                       // depthBiasConstantFactor
		0.0f,                       // depthBiasClamp
		0.0f,                       // depthBiasSlopeFactor
		1.0f                        // lineWidth
	);

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = vk::False;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = vk::True;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.colorWriteMask =
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.logicOpEnable = vk::False;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pSetLayouts = &*m_descriptorsetLayoutInfo;

	pipelineLayout = vk::raii::PipelineLayout(Device.getLogicalDevice(), pipelineLayoutInfo);
	vk::Format colorFormat = swapChainSurfaceFormat.format;
	vk::PipelineRenderingCreateInfo pipelineRenderCreateInfo{};
	pipelineRenderCreateInfo.colorAttachmentCount = 1;
	pipelineRenderCreateInfo.pColorAttachmentFormats = &colorFormat;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &pipelineRenderCreateInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = nullptr;

	graphicsPipeline = vk::raii::Pipeline(Device.getLogicalDevice(), nullptr, pipelineInfo);

}

