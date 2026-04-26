#pragma once
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include "vk_mem_alloc.h"

class LogicalDevice;
class VideoPlayer;

struct IconTexture {
    VkImage         image      = VK_NULL_HANDLE;
    VmaAllocation   allocation = VK_NULL_HANDLE;  
    VkImageView     imageView  = VK_NULL_HANDLE;
    VkSampler       sampler    = VK_NULL_HANDLE;
    ImTextureID     textureID  = (ImTextureID)0;
};

class ImguiRender {
public:
    void init(LogicalDevice& logicalDevice, vk::raii::DescriptorPool& imguiPool,
              VkInstance& instance, VkPhysicalDevice& physDevice,
              VkQueue& graphicsQueue, uint32_t swapchainImageCount,
              VkFormat swapchainFormat, VkRenderPass renderPass,
              GLFWwindow* window, const vk::raii::CommandBuffer& commandBuffer);

    
    IconTexture loadTexture(const std::string& path);

    void RenderImgui(const vk::raii::CommandBuffer& commandBuffer,
                     float width, float height, VideoPlayer& videoPlayer);

    void cleanupImgui();

    
    bool hasFileRequest() const { return m_fileRequested; }
    std::string consumeFileRequest() {
        m_fileRequested = false;
        std::string p = m_requestedPath;
        m_requestedPath.clear();
        return p;
    }

private:
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDevice     = VK_NULL_HANDLE;
    VkQueue          m_queue          = VK_NULL_HANDLE;
    VkCommandPool    m_commandPool    = VK_NULL_HANDLE;
    VmaAllocator     m_allocator      = VK_NULL_HANDLE;

    
    std::vector<IconTexture> m_textures;

    IconTexture m_playIcon;
    IconTexture m_pauseIcon;
    IconTexture m_forwardIcon;
    IconTexture m_backwardIcon;

        bool        m_fileRequested = false;
    std::string m_requestedPath;

    
    void submitSingleTimeCmd(std::function<void(VkCommandBuffer)> fn);
};