#include "LogicalDevice.hh"

class UBObuffer;

class GrapicPileline {
public:
	GrapicPileline();
	~GrapicPileline();
	void Init(LogicalDevice& Device, vk::Extent2D swapChainExtent, UBObuffer& ubodesptorset, vk::SurfaceFormatKHR swapChainSurfaceFormat);


	const vk::raii::Pipeline& GetPipeline() const { return graphicsPipeline; }
	const vk::raii::PipelineLayout& GetPipelineLayout() const { return pipelineLayout; }

private:
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline       graphicsPipeline = nullptr;
};