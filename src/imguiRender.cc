#include "imguiRender.hh"
#include "LogicalDevice.hh"
#include "VideoPlayer.hh"
#include <volk.h>
#include <stdexcept>
#include <functional>
#include "stb_image.h"


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <string>

static std::string OpenVideoFileDialog()
{
    char filename[MAX_PATH] = {};
    OPENFILENAMEA ofn       = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = NULL;
    ofn.lpstrFilter  = "Video Files\0*.mp4;*.mkv;*.avi;*.webm;*.mov;*.flv;*.wmv;*.mpg;*.mpeg;*.ts;*.m4v;*.3gp\0"
                       "All Files\0*.*\0";
    ofn.lpstrFile    = filename;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = "Open Video";
    ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
        return std::string(filename);
    return {};
}



void ImguiRender::submitSingleTimeCmd(std::function<void(VkCommandBuffer)> fn)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    fn(cmd);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;
    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

IconTexture ImguiRender::loadTexture(const std::string& path)
{
    int w, h, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error("ImguiRender::loadTexture: failed to load " + path);

    VkDeviceSize imageSize = (VkDeviceSize)w * h * 4;

    // Staging buffer via VMA 
    VkBuffer       stagingBuf;
    VmaAllocation  stagingAlloc;
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size        = imageSize;
        bufInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        vmaCreateBuffer(m_allocator, &bufInfo, &allocCI, &stagingBuf, &stagingAlloc, nullptr);

        void* data;
        vmaMapMemory(m_allocator, stagingAlloc, &data);
        memcpy(data, pixels, (size_t)imageSize);
        vmaUnmapMemory(m_allocator, stagingAlloc);
    }
    stbi_image_free(pixels);

    //  Image  VMA 
    IconTexture out{};
    {
        VkImageCreateInfo imgInfo{};
        imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType     = VK_IMAGE_TYPE_2D;
        imgInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
        imgInfo.extent        = { (uint32_t)w, (uint32_t)h, 1 };
        imgInfo.mipLevels     = 1;
        imgInfo.arrayLayers   = 1;
        imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imgInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(m_allocator, &imgInfo, &allocCI, &out.image, &out.allocation, nullptr);
    }

   
    submitSingleTimeCmd([&](VkCommandBuffer cmd) {
        
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = out.image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent                 = { (uint32_t)w, (uint32_t)h, 1 };
        vkCmdCopyBufferToImage(cmd, stagingBuf, out.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // TransferDst ShaderReadOnly
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    });

    // Clean up staging
    vmaDestroyBuffer(m_allocator, stagingBuf, stagingAlloc);

 
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = out.image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.layerCount     = 1;
        vkCreateImageView(m_device, &viewInfo, nullptr, &out.imageView);
    }

    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter    = VK_FILTER_LINEAR;
        samplerInfo.minFilter    = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(m_device, &samplerInfo, nullptr, &out.sampler);
    }


    out.textureID = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        out.sampler, out.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    m_textures.push_back(out);
    return out;
}



void ImguiRender::init(LogicalDevice& logicalDevice, vk::raii::DescriptorPool& imguiPool,
                        VkInstance& instance, VkPhysicalDevice& physDevice,
                        VkQueue& graphicsQueue, uint32_t swapchainImageCount,
                        VkFormat swapchainFormat, VkRenderPass /*renderPass*/,
                        GLFWwindow* window, const vk::raii::CommandBuffer& commandBuffer)
{
    // store raw handles needed by loadTexture / submitSingleTimeCmd
    m_device     = *logicalDevice.getLogicalDevice();
    m_physDevice = physDevice;
    m_queue      = graphicsQueue;
    m_allocator  = logicalDevice.GetAllocator();

  
    VkCommandBufferAllocateInfo tmpInfo{};
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = logicalDevice.getQueueIndex();
    vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

   
    std::vector<vk::DescriptorPoolSize> pool_sizes = {
        { vk::DescriptorType::eCombinedImageSampler, 16 }  
    };
    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 16, pool_sizes);
    imguiPool = vk::raii::DescriptorPool(logicalDevice.getLogicalDevice(), pool_info);

    
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance       = instance;
    init_info.PhysicalDevice = physDevice;
    init_info.Device         = m_device;
    init_info.Queue          = graphicsQueue;
    init_info.DescriptorPool = *imguiPool;
    init_info.MinImageCount  = swapchainImageCount;
    init_info.ImageCount     = swapchainImageCount;
    init_info.UseDynamicRendering = true;

    VkPipelineRenderingCreateInfoKHR pipeline_rendering_info{};
    pipeline_rendering_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipeline_rendering_info.colorAttachmentCount    = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &swapchainFormat;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeline_rendering_info;

    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3,
        [](const char* fn, void* ud) -> PFN_vkVoidFunction {
            return vkGetInstanceProcAddr(*reinterpret_cast<VkInstance*>(ud), fn);
        }, &instance);

    ImGui_ImplVulkan_Init(&init_info);

  
    auto tryLoad = [&](const std::string& path) -> IconTexture {
        try { return loadTexture(path); }
        catch (...) { return IconTexture{}; }  
    };

    m_pauseIcon    = tryLoad("assets/icons/pause.png");
    m_playIcon     = tryLoad("assets/icons/play.png");
    m_forwardIcon  = tryLoad("assets/icons/forward.png");
    m_backwardIcon = tryLoad("assets/icons/backward.png");
}



static void FormatTime(float seconds, char* buf, int bufSize)
{
    if (seconds < 0.f) seconds = 0.f;
    int totalSec = (int)seconds;
    int m = totalSec / 60;
    int s = totalSec % 60;
    snprintf(buf, bufSize, "%02d:%02d", m, s);
}

void ImguiRender::RenderImgui(const vk::raii::CommandBuffer& commandBuffer,
                               float width, float height, VideoPlayer& videoPlayer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

  
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

  
    ImVec4 origBtn       = colors[ImGuiCol_Button];
    ImVec4 origBtnHov    = colors[ImGuiCol_ButtonHovered];
    ImVec4 origBtnAct    = colors[ImGuiCol_ButtonActive];
    ImVec2 origPad       = style.FramePadding;
    float  origRound     = style.FrameRounding;

  
    colors[ImGuiCol_Button]        = ImVec4(1.f, 1.f, 1.f, 0.08f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.f, 1.f, 1.f, 0.20f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(1.f, 1.f, 1.f, 0.35f);
    style.FrameRounding = 6.f;

  
    const float CONTROLS_H = 90.f;
    const float PAD_X = 16.f;
    ImGui::SetNextWindowPos({ 0.f, height - CONTROLS_H });
    ImGui::SetNextWindowSize({ width, CONTROLS_H });
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(PAD_X, 8.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##controls", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing);

    bool playing = videoPlayer.isPlaying();
    float totalSeconds   = (float)videoPlayer.getTotalDuration();
    float currentSeconds = (float)videoPlayer.getProgressBar();
    if (totalSeconds <= 0.f) totalSeconds = 1.f;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos  = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    
    ImU32 colTop    = IM_COL32(0, 0, 0, 0);
    ImU32 colBottom = IM_COL32(0, 0, 0, 10);
    draw->AddRectFilledMultiColor(
        winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        colTop, colTop, colBottom, colBottom);

    
    const float barY      = winPos.y + 10.f;
    const float barH      = 5.f;
    const float barLeft   = winPos.x + PAD_X;
    const float barRight  = winPos.x + winSize.x - PAD_X;
    const float barWidth  = barRight - barLeft;
    float fraction        = currentSeconds / totalSeconds;
    if (fraction > 1.f) fraction = 1.f;
    if (fraction < 0.f) fraction = 0.f;
    const float filledRight = barLeft + barWidth * fraction;

    draw->AddRectFilled(
        ImVec2(barLeft, barY), ImVec2(barRight, barY + barH),
        IM_COL32(255, 255, 255, 40), 2.f);

    
    if (fraction > 0.001f) {
        draw->AddRectFilledMultiColor(
            ImVec2(barLeft, barY), ImVec2(filledRight, barY + barH),
            IM_COL32(66, 133, 244, 255), IM_COL32(0, 200, 255, 255),
            IM_COL32(0, 200, 255, 255), IM_COL32(66, 133, 244, 255));
    }

    const float thumbRadius = 7.f;
    ImVec2 thumbCenter(filledRight, barY + barH * 0.5f);

    ImVec2 barHitMin(barLeft, barY - 8.f);
    ImVec2 barHitMax(barRight, barY + barH + 8.f);
    ImGui::SetCursorScreenPos(barHitMin);
    ImGui::InvisibleButton("##seekbar", ImVec2(barWidth, barH + 16.f));
    bool barHovered = ImGui::IsItemHovered();
    bool barActive  = ImGui::IsItemActive();

    if (barActive) {
       
        float mouseX = ImGui::GetIO().MousePos.x;
        float newFrac = (mouseX - barLeft) / barWidth;
        if (newFrac < 0.f) newFrac = 0.f;
        if (newFrac > 1.f) newFrac = 1.f;
        currentSeconds = newFrac * totalSeconds;
        fraction = newFrac;
    }

    if (ImGui::IsItemDeactivated()) {
        float mouseX = ImGui::GetIO().MousePos.x;
        float newFrac = (mouseX - barLeft) / barWidth;
        if (newFrac < 0.f) newFrac = 0.f;
        if (newFrac > 1.f) newFrac = 1.f;
        videoPlayer.setProgressBar((double)(newFrac * totalSeconds));
    }

    float drawRadius = (barHovered || barActive) ? thumbRadius : thumbRadius - 2.f;
    ImVec2 actualThumb(barLeft + barWidth * fraction, barY + barH * 0.5f);
    
    if (barHovered || barActive) {
        draw->AddCircleFilled(actualThumb, drawRadius + 4.f, IM_COL32(66, 133, 244, 50));
    }
    draw->AddCircleFilled(actualThumb, drawRadius, IM_COL32(255, 255, 255, 240));

   
    char timeCurrent[16], timeTotal[16];
    FormatTime(currentSeconds, timeCurrent, sizeof(timeCurrent));
    FormatTime(totalSeconds, timeTotal, sizeof(timeTotal));

    const float timeY = barY + barH + 4.f;
    draw->AddText(ImVec2(barLeft, timeY), IM_COL32(200, 200, 200, 220), timeCurrent);

    ImVec2 totalSize = ImGui::CalcTextSize(timeTotal);
    draw->AddText(ImVec2(barRight - totalSize.x, timeY), IM_COL32(200, 200, 200, 220), timeTotal);

  
    const float btnSize = 32.f;
    const float btnSpacing = 20.f;
    const float totalBtnWidth = btnSize * 3 + btnSpacing * 2;
    const float btnStartX = (winSize.x - totalBtnWidth) * 0.5f;
    const float btnY = timeY + 20.f;

    ImGui::SetCursorPos(ImVec2(btnStartX, btnY - winPos.y));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

    if (m_backwardIcon.textureID) {
        if (ImGui::ImageButton("##bwd", m_backwardIcon.textureID, ImVec2(btnSize, btnSize)))
            videoPlayer.backward5Seconds();
    } else {
        if (ImGui::Button("<<", ImVec2(btnSize, btnSize)))
            videoPlayer.backward5Seconds();
    }
    ImGui::SameLine(0.f, btnSpacing);


    const float playSize = btnSize + 8.f;
    const ImTextureID playPauseIcon = playing
        ? m_pauseIcon.textureID
        : m_playIcon.textureID;

    float playBtnY = ImGui::GetCursorPosY() - 4.f;
    ImGui::SetCursorPosY(playBtnY);

    if (playPauseIcon) {
        if (ImGui::ImageButton("##play", playPauseIcon, ImVec2(playSize, playSize)))
            videoPlayer.playpause();
    } else {
        if (ImGui::Button(playing ? "||" : ">", ImVec2(playSize, playSize)))
            videoPlayer.playpause();
    }
    ImGui::SameLine(0.f, btnSpacing);

    
    ImGui::SetCursorPosY(playBtnY + 4.f); 
    if (m_forwardIcon.textureID) {
        if (ImGui::ImageButton("##fwd", m_forwardIcon.textureID, ImVec2(btnSize, btnSize)))
            videoPlayer.forward5Seconds();
    } else {
        if (ImGui::Button(">>", ImVec2(btnSize, btnSize)))
            videoPlayer.forward5Seconds();
    }

    // Open File button — right-aligned
    ImGui::SameLine(winSize.x - PAD_X - 70.f);
    ImGui::SetCursorPosY(playBtnY + 4.f);
    if (ImGui::Button("Open", ImVec2(60.f, btnSize))) {
        std::string path = OpenVideoFileDialog();
        if (!path.empty()) {
            m_fileRequested = true;
            m_requestedPath = path;
        }
    }

    ImGui::PopStyleColor(1);  
    ImGui::PopStyleVar(3);    

    ImGui::PopStyleVar(2); 
    ImGui::End();

   
    colors[ImGuiCol_Button]        = origBtn;
    colors[ImGuiCol_ButtonHovered] = origBtnHov;
    colors[ImGuiCol_ButtonActive]  = origBtnAct;
    style.FramePadding  = origPad;
    style.FrameRounding = origRound;

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);
}

void ImguiRender::cleanupImgui()
{
    if (!ImGui::GetCurrentContext())
        return;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    
    for (auto& tex : m_textures) {
        if (tex.sampler)   vkDestroySampler(m_device, tex.sampler, nullptr);
        if (tex.imageView) vkDestroyImageView(m_device, tex.imageView, nullptr);
        if (tex.image)     vmaDestroyImage(m_allocator, tex.image, tex.allocation);
    }
    m_textures.clear();

    if (m_commandPool) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}
