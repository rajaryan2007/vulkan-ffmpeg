#pragma once
#include <GLFW/glfw3.h>
#include "Logger.h"  
#include "instance.hh"
#include "physicalDevice.hh"
#include "LogicalDevice.hh"
#include "swapchain.hh"
#include "GrapicPipeline.hh"
#include "CommandPool.hh"
#include "vertexBuffer.hh"
#include "index_buffer.hh"
#include "Texture.hh"
#include "VideoPlayer.hh"
#include "utils.hh"
#include <memory>
#include <utility>
#include <chrono>
#include "imguiRender.hh"



class Application {
public:
	Application();
	void run();

	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();
	void drawFrame();
    void createSyncObjects( );
	void recreateSwapChain();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	
private:
	
	const std::vector<Vertex> vertices = {
		{glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}, // Bottom-Left
		{glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)}, // Bottom-Right
		{glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)}, // Top-Right
		{glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)}, // Top-Left
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};
	GLFWwindow* m_window;
	VulkanInstance m_instance;
	PhysicalDevice m_physicalDevice;
	LogicalDevice m_logicalDevice;
	UBObuffer m_Uniformbuffer;


	Swapchain m_swapchain;
	GrapicPileline m_graphicPipeline;
	VertexBuffer m_vertexBuffer;
	std::shared_ptr<CommandPool> m_CommandPool;
	std::unique_ptr<TextureVK> m_textureVk;
	std::unique_ptr<VideoPlayer> m_videoPlayer;
	ImguiRender m_imguiRender;
	vk::raii::DescriptorPool m_imguiPool = nullptr;
	uint32_t& frameIndex = m_CommandPool->GetFrameIndex();
	std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	
	std::vector<vk::raii::Fence> inFlightFences;

	// Video frame timing
	std::chrono::high_resolution_clock::time_point lastFrameTime;
	double frameAccumulator = 0.0;
	bool m_videoLoaded = false;
	
};
