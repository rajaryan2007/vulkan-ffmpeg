#include "index_buffer.hh"
#include "CommandPool.hh"
#include "LogicalDevice.hh"
#include "physicalDevice.hh"
#include "swapchain.hh"
#include "vertexBuffer.hh"
#include <chrono>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <stdexcept>

void UBObuffer::createDescriptorSetLayout(LogicalDevice &logicaldev,
                                          uint32_t MAX_FRAMES_IN_FLIGHT) {

  const auto &device = logicaldev.getLogicalDevice();

  // define the bindings: UBO at binding 0, Sampler at binding 1
  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,
                                     vk::ShaderStageFlagBits::eVertex, nullptr),
      vk::DescriptorSetLayoutBinding(
          1, vk::DescriptorType::eCombinedImageSampler, 1,
          vk::ShaderStageFlagBits::eFragment, nullptr)};

  //  Create the Layout
  
  vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
  descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

  
  // you defined the info for this, but didn't instantiate the pool.
  std::array<vk::DescriptorPoolSize, 2> poolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer,
                             MAX_FRAMES_IN_FLIGHT),
      vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
                             MAX_FRAMES_IN_FLIGHT)};

  vk::DescriptorPoolCreateInfo poolInfo(
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      MAX_FRAMES_IN_FLIGHT, poolSizes);

  
  // vk::raii::DescriptorPool: descriptorPool = vk::raii::DescriptorPool(device,
  // poolInfo);
}

UBObuffer::~UBObuffer() {
  if (m_allocator) {
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
      if (uniformBuffersMapped[i]) {
        vmaUnmapMemory(m_allocator, uniformBuffersAllocation[i]);
      }
      vmaDestroyBuffer(m_allocator, *uniformBuffers[i],
                       uniformBuffersAllocation[i]);
      uniformBuffers[i].release();
    }

    if (indexBuffer != nullptr) {
      vmaDestroyBuffer(m_allocator, *indexBuffer, indexBufferAllocation);
      indexBuffer.release();
    }
  }
}

void UBObuffer::createUniformBuffers(CommandPool &m_commandPool,
                                     PhysicalDevice &physicalDev,
                                     LogicalDevice &logicalDev) {
  m_allocator = logicalDev.GetAllocator();
  const auto &MAX_FRAMES_IN_FLIGHT = m_commandPool.GetMaxFramesInFlight();

  uniformBuffers.clear();
  uniformBuffersAllocation.clear();
  uniformBuffersMapped.clear();

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    vk::raii::Buffer buffer(nullptr);
    VmaAllocation allocation;

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,VMA_MEMORY_USAGE_CPU_TO_GPU, buffer, allocation, logicalDev);

    uniformBuffers.emplace_back(std::move(buffer));
    uniformBuffersAllocation.emplace_back(allocation);

    void *mappedData;
    vmaMapMemory(logicalDev.GetAllocator(), allocation, &mappedData);
    uniformBuffersMapped.emplace_back(mappedData);
  }
}

void UBObuffer::updateUniformBuffer(uint32_t currentImage, float windowWidth, float windowHeight, float videoWidth, float videoHeight) {
  float windowAspect = windowWidth / windowHeight;
  float videoAspect = videoWidth / videoHeight;

  float scaleX = 1.0f;
  float scaleY = 1.0f;

  if (windowAspect > videoAspect) {
    // window is wider than video. Scale down width to fit height.
    scaleX = videoAspect / windowAspect;
  } else {
    // window is taller than video. scale down height to fit width.
    scaleY = windowAspect / videoAspect;
  }

  ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 1.0f));
  ubo.view = glm::mat4(1.0f);
  ubo.proj = glm::mat4(1.0f);
  // flip Y axis — Vulkan has Y-down, pipeline expects CCW front face
  ubo.proj[1][1] *= -1;

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void UBObuffer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                             VmaMemoryUsage memoryUsage,
                             vk::raii::Buffer &buffer,
                             VmaAllocation &allocation,
                             LogicalDevice &LogicalDev) {
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = (VkBufferUsageFlags)usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memoryUsage;

  VkBuffer vkBuffer;
  if (vmaCreateBuffer(LogicalDev.GetAllocator(), &bufferInfo, &allocInfo,
                      &vkBuffer, &allocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer via VMA!");
  }

  buffer = vk::raii::Buffer(LogicalDev.getLogicalDevice(), vkBuffer);
}

uint32_t UBObuffer::findMemoryType(uint32_t typeFilter,
                                   vk::MemoryPropertyFlags properties,
                                   PhysicalDevice physicalDevice) {
  vk::PhysicalDeviceMemoryProperties memProperties =
      physicalDevice.device().getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void UBObuffer::createDescriptorPool(LogicalDevice &logicalDev,
                                     const CommandPool &commandPool,
                                     uint32_t MAX_FRAMES_IN_FLIGHT) {

  //const auto& MAX_FRAMES_IN_FLIGHT = commandPool.GetMaxFramesInFlight();

  std::array<vk::DescriptorPoolSize, 2> poolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer,
                             MAX_FRAMES_IN_FLIGHT),
      vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
                             MAX_FRAMES_IN_FLIGHT)};

  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();

  descriptorPool =
      vk::raii::DescriptorPool(logicalDev.getLogicalDevice(), poolInfo);
}

void UBObuffer::createDescriptorSets(LogicalDevice &logicalDev,
                                     const CommandPool &commandPool,
                                     uint32_t MAX_FRAMES_IN_FLIGHT,
                                     const Swapchain &swapchain) {
  std::vector<vk::DescriptorSetLayout> layouts(
      commandPool.GetMaxFramesInFlight(), *descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo{};
  allocInfo.descriptorPool = descriptorPool,
  allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets =
      logicalDev.getLogicalDevice().allocateDescriptorSets(allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i], bufferInfo.offset = 0,
    bufferInfo.range = sizeof(UniformBufferObject);

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = *swapchain.getTextureImageView();
    imageInfo.sampler = *swapchain.getTextureSampler();

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        vk::DescriptorType::eCombinedImageSampler;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    logicalDev.getLogicalDevice().updateDescriptorSets(descriptorWrites, {});
  }
}
