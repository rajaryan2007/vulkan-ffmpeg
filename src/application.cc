#include "application.hh"
#include <glm/glm.hpp>

constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 1080;

void Application::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

Application::Application()
    : m_window(nullptr), presentCompleteSemaphores(),
      renderFinishedSemaphores(), inFlightFences(),

      m_CommandPool(std::make_shared<CommandPool>())

{}
void Application::initVulkan() {

  LOG("Vulkan initialized.");

  m_instance.createInstance();

  m_logicalDevice.createSurface(m_instance, m_window);
  m_physicalDevice.createPhysicalDevice(m_instance);
  m_logicalDevice.findLogicaldevice(m_physicalDevice, m_instance);
  m_swapchain.createSwapChain(m_physicalDevice, m_logicalDevice, *m_window);
  m_swapchain.createImageViews(m_logicalDevice);

  m_Uniformbuffer.createDescriptorSetLayout(
      m_logicalDevice, m_CommandPool->GetMaxFramesInFlight());
  m_graphicPipeline.Init(m_logicalDevice, m_swapchain.GetExtent(),
                         m_Uniformbuffer, m_swapchain.GetSurfaceFormat());

  m_CommandPool->Init(m_logicalDevice);

  // video player setup 
  m_videoPlayer = std::make_unique<VideoPlayer>();

  m_textureVk = std::make_unique<TextureVK>();
  m_textureVk->setAllocator(m_logicalDevice.GetAllocator());
  m_textureVk->CreateTextureForVideo(m_Uniformbuffer, m_logicalDevice, *m_CommandPool, 1, 1);
  {
      uint8_t black[4] = {0, 0, 0, 255};
      m_textureVk->updateTexture(black, 1, 1, m_logicalDevice, *m_CommandPool);
  }

  m_swapchain.createTextureImageView(m_logicalDevice, *m_textureVk);
  m_swapchain.createTextureSampler(m_logicalDevice, m_physicalDevice);
  m_Uniformbuffer.createUniformBuffers(*m_CommandPool, m_physicalDevice,m_logicalDevice);
  m_Uniformbuffer.createDescriptorPool(m_logicalDevice, *m_CommandPool,m_CommandPool->GetMaxFramesInFlight());
  m_Uniformbuffer.createDescriptorSets(m_logicalDevice, *m_CommandPool,m_CommandPool->GetMaxFramesInFlight(),m_swapchain);

  const auto &commmandPool = m_CommandPool->GetCommandPool();
  m_vertexBuffer.createVertexBuffer(commmandPool, m_physicalDevice,m_logicalDevice, vertices);
  m_vertexBuffer.createIndexBuffer(commmandPool, m_physicalDevice,m_logicalDevice, indices);
  m_CommandPool->createCommandBuffer(m_logicalDevice);
  createSyncObjects();

  VkInstance vkInst = *m_instance.instance();
  VkPhysicalDevice vkPhys = *m_physicalDevice.device();
  VkQueue vkQueue = *m_logicalDevice.GetQueue();
  m_imguiRender.init(
m_logicalDevice, m_imguiPool, vkInst, vkPhys, vkQueue,
      static_cast<uint32_t>(m_swapchain.GetImage().size()),
      static_cast<VkFormat>(m_swapchain.GetSurfaceFormat().format), nullptr,
      m_window, m_CommandPool->GetCommandBuffer());

  lastFrameTime = std::chrono::high_resolution_clock::now();
}

void Application::mainLoop() {
  LOG("Entering main loop.");
  while (!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();
    drawFrame();
  }
  m_logicalDevice.getLogicalDevice().waitIdle();
}

void Application::cleanup() {

  m_imguiRender.cleanupImgui();

  
  m_textureVk.reset();
  m_videoPlayer.reset();
  m_CommandPool.reset();

  glfwDestroyWindow(m_window);
  glfwTerminate();
  LOG("Cleanup complete.");
}

void Application::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(m_window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(m_window, &width, &height);
    glfwWaitEvents();
  }
  auto &device = m_logicalDevice.getLogicalDevice();
  device.waitIdle();

  m_swapchain.cleanupSwapChain();

  m_swapchain.createSwapChain(m_physicalDevice, m_logicalDevice, *m_window);
  m_swapchain.createImageViews(m_logicalDevice);
}

void Application::framebufferResizeCallback(GLFWwindow *window, int width,
                                            int height) {
  auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->recreateSwapChain();
}

void Application::initWindow() {
  LOG("window complete.");

  glfwInit();
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Player", nullptr, nullptr);
  if (m_window == nullptr) {
    throw std::runtime_error("Failed to create GLFW window");
  };

  glfwSetWindowUserPointer(m_window, this);
  glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}
void Application::drawFrame() {
  
  const auto &queue = m_logicalDevice.GetQueue();
  const auto &device = m_logicalDevice.getLogicalDevice();
  auto MAX_FRAMES_IN_FLIGHT = m_CommandPool->GetMaxFramesInFlight();
  double syncThreeshold = 0.1;
  double audioTime = m_videoPlayer->getAudioClock();
  double videoTime = m_videoPlayer->getVideoClock();
  double diff = videoTime - audioTime;
  
  queue.waitIdle();
  const auto &swapChain = m_swapchain.getSwapChain();

  auto fenceResult =
      device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

  
  if (m_imguiRender.hasFileRequest()) {
      std::string path = m_imguiRender.consumeFileRequest();
      if (m_videoPlayer->open(path)) {
          m_videoPlayer->decodeNextFrame();
          // recreate texture at new video resolution
          m_textureVk = std::make_unique<TextureVK>();
          m_textureVk->setAllocator(m_logicalDevice.GetAllocator());
          m_textureVk->CreateTextureForVideo(m_Uniformbuffer, m_logicalDevice,
              *m_CommandPool, m_videoPlayer->getWidth(), m_videoPlayer->getHeight());
          m_textureVk->updateTexture(
              m_videoPlayer->getFrameData(), m_videoPlayer->getWidth(),
              m_videoPlayer->getHeight(), m_logicalDevice, *m_CommandPool);
          m_swapchain.createTextureImageView(m_logicalDevice, *m_textureVk);
          m_swapchain.createTextureSampler(m_logicalDevice, m_physicalDevice);
          // reset old descriptors before re-allocating
          m_Uniformbuffer.resetDescriptorPool(m_logicalDevice);
          m_Uniformbuffer.createDescriptorSets(m_logicalDevice, *m_CommandPool,
              m_CommandPool->GetMaxFramesInFlight(), m_swapchain);
          m_videoLoaded = true;
      }
  }

   
  if (m_videoLoaded) {
      //video frame timing using audio clock as master
      bool frameUploaded = false;
	  if (m_videoLoaded && m_videoPlayer->isPlaying()) {
		  double audioTime = m_videoPlayer->getAudioClock();
		  double videoTime = m_videoPlayer->getVideoClock();
		  double diff = videoTime - audioTime;

		  // sync_threshold: how much desync we tolerate (e.g., 10ms - 40ms)
		  double sync_threshold = std::max(0.01, 1.0 / m_videoPlayer->getFrameRate());

		  if (diff > sync_threshold) {
			  // video is too far ahead stay on the current frame
			
		  }
		  else {
			  //  decode the next frame.
			  if (m_videoPlayer->decodeNextFrame()) {
				  m_textureVk->updateTexture(
					  m_videoPlayer->getFrameData(), m_videoPlayer->getWidth(),
					  m_videoPlayer->getHeight(), m_logicalDevice, *m_CommandPool);
			  }

			  // If we are REALLY far behind 
			  // skip another frame immediately to catch up.
			  if (diff < -0.1) {
				  m_videoPlayer->decodeNextFrame();

			  }
		  }
	  }
  }

  auto extent = m_swapchain.GetExtent();
  float vidW = m_videoLoaded ? static_cast<float>(m_videoPlayer->getWidth()) : 1.f;
  float vidH = m_videoLoaded ? static_cast<float>(m_videoPlayer->getHeight()) : 1.f;
  m_Uniformbuffer.updateUniformBuffer(
      frameIndex, static_cast<float>(extent.width),
      static_cast<float>(extent.height), vidW, vidH);
  if (fenceResult != vk::Result::eSuccess) {
    throw std::runtime_error("failed to wait for fence!");
  }

  auto [result, imageIndex] = swapChain.acquireNextImage(
      UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  } else if (result != vk::Result::eSuccess &&
             result != vk::Result::eSuboptimalKHR) {
    assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  auto &vertexBuffer = m_vertexBuffer.get();
  const auto &IndexBuffer = m_vertexBuffer.getIndexBuffer();
  auto &indices = m_vertexBuffer.getIndices();
  m_CommandPool->recordCommandBuffer(vertexBuffer, m_graphicPipeline, imageIndex, m_swapchain, IndexBuffer,indices, m_Uniformbuffer.getDescriptorSets(), &m_imguiRender,&*m_videoPlayer);

  device.resetFences(*inFlightFences[frameIndex]);

  vk::PipelineStageFlags waitDestinationStageMask(
      vk::PipelineStageFlagBits::eColorAttachmentOutput);
  vk::SubmitInfo submitInfo{};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &*presentCompleteSemaphores[frameIndex];
  submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
  submitInfo.commandBufferCount = 1;

  submitInfo.pCommandBuffers = &*m_CommandPool->GetCommandBuffer();
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex];

  queue.submit(submitInfo, *inFlightFences[frameIndex]);
  result =
      device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  }
  if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
    assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  vk::PresentInfoKHR presentInfoKHR{};
  presentInfoKHR.waitSemaphoreCount = 1;
  presentInfoKHR.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex];
  presentInfoKHR.swapchainCount = 1;
  presentInfoKHR.pSwapchains = &*swapChain;
  presentInfoKHR.pImageIndices = &imageIndex;

  result = queue.presentKHR(presentInfoKHR);

  switch (result) {
  case vk::Result::eSuccess:
    break;
  case vk::Result::eSuboptimalKHR:
    std::cout
        << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
    break;
  default:
    break; // an unexpected result is returned!
  }
  frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::createSyncObjects() {
  const auto &MAX_FRAMES_IN_FLIGHT = m_CommandPool->GetMaxFramesInFlight();
  const auto &device = m_logicalDevice.getLogicalDevice();
  auto &swapChainImages = m_swapchain.GetImage();
  assert(presentCompleteSemaphores.empty());
  assert(renderFinishedSemaphores.empty());
  assert(inFlightFences.empty());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    inFlightFences.emplace_back(
        device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
  }
}
