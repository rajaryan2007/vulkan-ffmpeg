#include "utils.hh"



vk::raii::ShaderModule createShaderModule(const std::vector<char>& code,LogicalDevice& logical)
{  vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::raii::ShaderModule ShaderModule{ logical.getLogicalDevice(), createInfo };



	return ShaderModule;
}


