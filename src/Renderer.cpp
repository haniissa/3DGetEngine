#include "Renderer.h"
#include <algorithm>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fmt/base.h>
#include <fmt/format.h>
#include <SDL3/SDL_vulkan.h>
#include <fstream>
#include <cstring>
#include <vector>
#include <vulkan/vulkan_core.h>


#include "Renderer.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fstream>
#include <vector>

// Helper: read binary file
static std::vector<char> ReadFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if(!file.is_open())
        return {};
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();
    return buffer;
}

// Helper: find memory type  (NOTE: must loop over memoryTypeCount, not heap count)
static uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties){
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
        if((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
            return i;
        }
    }
    fmt::print(stderr, "[Renderer] Failed to find suitable memory type\n");
    return 0;
}

static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code){
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module = VK_NULL_HANDLE;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create shader module\n");
    }
    return module;
}

Renderer::~Renderer(){
    if(m_initialized) Shutdown();
}

bool Renderer::Initialize(SDL_Window* window, uint32_t width, uint32_t height){
    m_width = width;
    m_height = height;

    fmt::print("[Renderer] Initializing Vulkan...\n");

    if(!CreateInstance(window)) return false;
    if(!CreateSurface(window)) return false;
    if(!CreateDevice()) return false;
    if(!CreateSwapchain()) return false;
    if(!CreateDepthResources()) return false;
    if(!CreateRenderPass()) return false;
    if(!CreateDescriptorPool()) return false;
    if(!CreatePipelineLayout()) return false;
    if(!CreateGraphicsPipeline()) return false;
    if(!CreateFramebuffers()) return false;
    if(!CreateCommandPool()) return false;
    if(!CreateUniformBuffers()) return false;
    if(!CreateDescriptorSets()) return false;
    if(!CreateCommandBuffers()) return false;
    if(!CreateSyncObjects()) return false;

    m_initialized = true;
    fmt::print("[Renderer] Vulkan initialized successfully.\n");
    return true;
}

void Renderer::Shutdown(){
    if(!m_initialized) return;
    m_initialized = false;

    vkDeviceWaitIdle(m_device);

    // Cleanup uniform buffers
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr); // BUG FIX: was freeing m_indexBufferMemory here
    }

    // Cleanup mesh buffers
    if(m_vertexBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    }
    if(m_indexBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
    }

    // Cleanup sync objects
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
        vkDestroySemaphore(m_device, m_imageAvailable[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinished[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for(auto& fb : m_framebuffers){
        vkDestroyFramebuffer(m_device, fb, nullptr);
    }

    DestroyDepthResources();

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipeline(m_device, m_particlePipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for(auto& view : m_swapImageViews){
        vkDestroyImageView(m_device, view, nullptr);
    }

    vkb::destroy_swapchain(m_vkbSwapchain);
    vkb::destroy_device(m_vkbDevice);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkb::destroy_instance(m_vkbInstance);
}

bool Renderer::CreateInstance(SDL_Window* window){
    (void)window;
    vkb::InstanceBuilder builder;
    auto inst_ret = builder
        .set_app_name("3D Jet Engine Lab")
        .set_engine_name("JetEngineRenderer")
        .request_validation_layers(false) // Steam overlay errors come from validation layers; off for release
        .require_api_version(1, 3, 0)
        .build();
    if(!inst_ret){
        fmt::print(stderr, "[Renderer] Failed to create Vulkan instance: {}\n", inst_ret.error().message());
        return false;
    }
    m_vkbInstance = inst_ret.value();
    m_instance = m_vkbInstance.instance;
    return true;
}

bool Renderer::CreateSurface(SDL_Window* window){
    if(!SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &m_surface)){
        fmt::print(stderr, "[Renderer] Failed to create Vulkan surface: {}\n", SDL_GetError());
        return false;
    }
    return true;
}

bool Renderer::CreateDevice(){
    vkb::PhysicalDeviceSelector selector{m_vkbInstance};
    auto phys_ret = selector
        .set_surface(m_surface)
        .set_minimum_version(1, 3)
        .select();

    if(!phys_ret){
        fmt::print(stderr, "[Renderer] Failed to select physical device: {}\n", phys_ret.error().message());
        return false;
    }

    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if(!dev_ret){
        fmt::print(stderr, "[Renderer] Failed to create logical device: {}\n", dev_ret.error().message());
        return false;
    }

    m_vkbDevice = dev_ret.value();
    m_device = m_vkbDevice.device;
    m_physicalDevice = phys_ret.value().physical_device;

    auto graphics_queue = m_vkbDevice.get_queue(vkb::QueueType::graphics);
    auto present_queue = m_vkbDevice.get_queue(vkb::QueueType::present);
    if(!graphics_queue || !present_queue){
        fmt::print(stderr, "[Renderer] Failed to get queues\n");
        return false;
    }
    m_graphicsQueue = graphics_queue.value();
    m_presentQueue = present_queue.value();

    auto graphics_queue_family = m_vkbDevice.get_queue_index(vkb::QueueType::graphics);
    m_graphicsQueueFamily = graphics_queue_family.value();

    return true;
}

bool Renderer::CreateSwapchain(){
    vkb::SwapchainBuilder swapchain_builder{m_vkbDevice};
    auto swap_ret = swapchain_builder
        .set_desired_format(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // VSync
        .set_desired_extent(m_width, m_height)
        .build();

    if(!swap_ret){
        fmt::print(stderr, "[Renderer] Failed to create swapchain: {}\n", swap_ret.error().message());
        return false;
    }
    m_vkbSwapchain = swap_ret.value();
    m_swapchain = m_vkbSwapchain.swapchain;
    m_swapImages = m_vkbSwapchain.get_images().value();
    m_swapImageViews = m_vkbSwapchain.get_image_views().value();
    m_swapExtent = m_vkbSwapchain.extent;
    m_swapFormat = m_vkbSwapchain.image_format;

    m_imagesInFlight.resize(m_swapImages.size(), VK_NULL_HANDLE);
    return true;
}

bool Renderer::CreateDepthResources(){
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {m_swapExtent.width, m_swapExtent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create depth image\n");
        return false;
    }

    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(m_device, m_depthImage, &memReq);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(m_physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthImageMemory) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to allocate depth memory\n");
        return false;
    }
    vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if(vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create depth image view\n");
        return false;
    }
    return true;
}

void Renderer::DestroyDepthResources(){
    if(m_depthImageView != VK_NULL_HANDLE){ vkDestroyImageView(m_device, m_depthImageView, nullptr); m_depthImageView = VK_NULL_HANDLE; }
    if(m_depthImage != VK_NULL_HANDLE){ vkDestroyImage(m_device, m_depthImage, nullptr); m_depthImage = VK_NULL_HANDLE; }
    if(m_depthImageMemory != VK_NULL_HANDLE){ vkFreeMemory(m_device, m_depthImageMemory, nullptr); m_depthImageMemory = VK_NULL_HANDLE; }
}

bool Renderer::CreateRenderPass(){
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create render pass\n");
        return false;
    }
    return true;
}

bool Renderer::CreateDescriptorPool(){
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create descriptor pool\n");
        return false;
    }
    return true;
}

bool Renderer::CreatePipelineLayout(){
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create descriptor set layout\n");
        return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

    if(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create pipeline layout\n");
        return false;
    }

    return true;
}

bool Renderer::CreatePipeline(bool particlePass, VkPipeline& outPipeline){
    auto vertShaderCode = ReadFile("shaders/shader.vert.spv");
    auto fragShaderCode = ReadFile("shaders/shader.frag.spv");

    if(vertShaderCode.empty() || fragShaderCode.empty()){
        fmt::print(stderr, "[Renderer] ERROR: shader files not found in ./shaders/\n");
        fmt::print(stderr, "  Compile them with the Vulkan SDK's glslangValidator:\n");
        fmt::print(stderr, "    glslangValidator -V shaders/shader.vert -o shaders/shader.vert.spv\n");
        fmt::print(stderr, "    glslangValidator -V shaders/shader.frag -o shaders/shader.frag.spv\n");
        return false;
    }

    VkShaderModule vertModule = CreateShaderModule(m_device, vertShaderCode);
    VkShaderModule fragModule = CreateShaderModule(m_device, fragShaderCode);
    if(vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) return false;

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // Vertex input matching the Vertex struct exactly
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[5]{};
    attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
    attributeDescriptions[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)};
    attributeDescriptions[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
    attributeDescriptions[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, texCoord)};
    attributeDescriptions[4] = {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT,    offsetof(Vertex, extra)};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 5;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // lathe normals are approximations; keep everything visible
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    //Particles are drawn as on x-ray glow so you can see the flow through
    //the casing. Flip this to VK_TRUE if you prefer strict occlusion
    depthStencil.depthTestEnable = particlePass ? VK_FALSE : VK_TRUE;
    depthStencil.depthWriteEnable = particlePass ? VK_TRUE : VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    if(particlePass){
    	colorBlendAttachment.blendEnable = VK_TRUE;
    	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }else{
    	//Opaque metal. Blending here would haze overlapping lather shels.
     	colorBlendAttachment.blendEnable = VK_FALSE;
      	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
       	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline);

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);

    if(result != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create graphics pipeline\n");
        return false;
    }

    fmt::print("[Renderer] Graphics pipeline created.\n");
    return true;
}
bool Renderer::CreateGraphicsPipeline(){
	//Two passes over the same vertex buffer;
	// mesh - opaeue metal, depth write on
	// particles - additive glow, depth writ off
	return CreatePipeline(false, m_graphicsPipeline)
	and CreatePipeline(true, m_particlePipeline);
}

bool Renderer::CreateFramebuffers(){
    m_framebuffers.resize(m_swapImageViews.size());
    for(size_t i = 0; i < m_swapImageViews.size(); ++i){
        VkImageView attachments[] = {m_swapImageViews[i], m_depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapExtent.width;
        framebufferInfo.height = m_swapExtent.height;
        framebufferInfo.layers = 1;

        if(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to create framebuffer\n");
            return false;
        }
    }
    return true;
}

bool Renderer::CreateCommandPool(){
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;

    if(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to create command pool\n");
        return false;
    }
    return true;
}

bool Renderer::CreateCommandBuffers(){
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to allocate command buffers\n");
        return false;
    }
    return true;
}

bool Renderer::CreateSyncObjects(){
    m_imageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
        if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS ||
           vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS ||
           vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to create synchronization object\n");
            return false;
        }
    }
    return true;
}

bool Renderer::CreateUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_uniformBuffers[i]) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to create uniform buffer\n");
            return false;
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(m_device, m_uniformBuffers[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_physicalDevice, memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_uniformBuffersMemory[i]) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to allocate uniform buffer memory\n");
            return false;
        }
        vkBindBufferMemory(m_device, m_uniformBuffers[i], m_uniformBuffersMemory[i], 0);
        vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBufferMapped[i]);
    }
    return true;
}

bool Renderer::CreateDescriptorSets(){
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; // BUG FIX: was LAYOUT_CREATE_INFO
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if(vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to allocate descriptor sets\n");
        return false;
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }
    return true;
}

bool Renderer::BeginFrame(){
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // BUG FIX: the swapchain image was never acquired before
    VkResult acquireResult = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
        m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

    if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR){
        RecreateSwapchainInternal();
        return false; // skip this frame
    }
    if(acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR){
        fmt::print(stderr, "[Renderer] Failed to acquire swapchain image\n");
        return false;
    }

    // Track which fence guards which swapchain image
    if(m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE){
        vkWaitForFences(m_device, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[m_imageIndex] = m_inFlightFences[m_currentFrame];

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; // BUG FIX: was COMMAND_BUFFER_BEGIN_INFO
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapExtent;

    VkClearValue clearValues[2]{};
    clearValues[0].color = {{0.05F, 0.05F, 0.1F, 1.0F}};   // dark blue background
    clearValues[1].depthStencil = {1.0F, 0};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    return true;
}

void Renderer::EndFrame(){
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailable[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = {m_renderFinished[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_imageIndex;

    VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR){
        RecreateSwapchainInternal();
    } else if(presentResult != VK_SUCCESS){
        fmt::print(stderr, "[Renderer] Failed to present swapchain image\n");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool Renderer::RecreateSwapchain(uint32_t width, uint32_t height){
    if(width == 0 || height == 0) return false;
    if(width == m_width && height == m_height) return true;
    m_width = width;
    m_height = height;
    RecreateSwapchainInternal();
    return true;
}

void Renderer::RecreateSwapchainInternal(){
    vkDeviceWaitIdle(m_device);

    for(auto& fb : m_framebuffers){
        vkDestroyFramebuffer(m_device, fb, nullptr);
    }
    m_framebuffers.clear();
    DestroyDepthResources();

    vkb::SwapchainBuilder builder{m_vkbDevice};
    auto ret = builder
        .set_desired_format(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_old_swapchain(m_vkbSwapchain)
        .build();

    if(!ret){
        fmt::print(stderr, "[Renderer] Failed to recreate swapchain: {}\n", ret.error().message());
        return;
    }

    vkb::destroy_swapchain(m_vkbSwapchain);
    m_vkbSwapchain = ret.value();
    m_swapchain = m_vkbSwapchain.swapchain;
    m_swapImages = m_vkbSwapchain.get_images().value();
    m_swapImageViews = m_vkbSwapchain.get_image_views().value();
    m_swapExtent = m_vkbSwapchain.extent;
    m_swapFormat = m_vkbSwapchain.image_format;

    m_imagesInFlight.clear();
    m_imagesInFlight.resize(m_swapImages.size(), VK_NULL_HANDLE);

    CreateDepthResources();
    CreateFramebuffers();
}

void Renderer::UploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices){
    VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    // Grow vertex buffer if needed (persistently mapped, host coherent)
    if(vertexSize > m_vertexBufferCapacity){
        if(m_vertexBuffer != VK_NULL_HANDLE){
            vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
            vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
            m_vertexBuffer = VK_NULL_HANDLE;
        }
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = std::max<VkDeviceSize>(vertexSize, 1 << 20); // 1 MB minimum
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to create vertex buffer\n");
            return;
        }
        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_physicalDevice, memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to allocate vertex buffer memory\n");
            return;
        }
        vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);
        vkMapMemory(m_device, m_vertexBufferMemory, 0, bufferInfo.size, 0, &m_vertexBufferMapped);
        m_vertexBufferCapacity = bufferInfo.size;
    }

    if(indexSize > m_indexBufferCapacity){
        if(m_indexBuffer != VK_NULL_HANDLE){
            vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
            vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
            m_indexBuffer = VK_NULL_HANDLE;
        }
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = std::max<VkDeviceSize>(indexSize, 1 << 18); // 256 KB minimum
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_indexBuffer) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to create index buffer\n");
            return;
        }
        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(m_device, m_indexBuffer, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_physicalDevice, memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS){
            fmt::print(stderr, "[Renderer] Failed to allocate index buffer memory\n");
            return;
        }
        vkBindBufferMemory(m_device, m_indexBuffer, m_indexBufferMemory, 0);
        vkMapMemory(m_device, m_indexBufferMemory, 0, bufferInfo.size, 0, &m_indexBufferMapped);
        m_indexBufferCapacity = bufferInfo.size;
    }

    if(m_vertexBufferMapped && vertexSize > 0){
        memcpy(m_vertexBufferMapped, vertices.data(), vertexSize);
    }
    if(m_indexBufferMapped && indexSize > 0){
        memcpy(m_indexBufferMapped, indices.data(), indexSize);
    }
    m_indexCount = static_cast<uint32_t>(indices.size());
}

// void Renderer::DrawMesh(uint32_t indexCount, uint32_t instanceCount){
//     if(m_graphicsPipeline == VK_NULL_HANDLE || m_vertexBuffer == VK_NULL_HANDLE || m_indexBuffer == VK_NULL_HANDLE){
//         return;
//     }
//     VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

//     vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

//     VkDeviceSize offsets[] = {0};
//     vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, offsets);
//     vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

//     vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
//         0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

//     VkViewport viewport{};
//     viewport.x = 0.0F;
//     viewport.y = 0.0F;
//     viewport.width = static_cast<float>(m_swapExtent.width);
//     viewport.height = static_cast<float>(m_swapExtent.height);
//     viewport.minDepth = 0.0F;
//     viewport.maxDepth = 1.0F;
//     vkCmdSetViewport(cmd, 0, 1, &viewport);

//     VkRect2D scissor{};
//     scissor.offset = {0, 0};
//     scissor.extent = m_swapExtent;
//     vkCmdSetScissor(cmd, 0, 1, &scissor);

//     vkCmdDrawIndexed(cmd, indexCount, instanceCount, 0, 0, 0);
// }

void Renderer::DrawMesh(uint32_t indexCount, uint32_t firstIndex, bool particlePass){
	if(indexCount == 0) return;
	if(m_vertexBuffer == VK_NULL_HANDLE or m_indexBuffer == VK_NULL_HANDLE) return;

	VkPipeline pipeline = particlePass ? m_particlePipeline : m_graphicsPipeline;
	if(pipeline == VK_NULL_HANDLE) return;

	VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, offsets);
	vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

	VkViewport viewport{};
	viewport.x = 0.0F;
	viewport.y = 0.0F;
	viewport.width = static_cast<float>(m_swapExtent.width);
	viewport.height = static_cast<float>(m_swapExtent.height);
	viewport.minDepth = 0.0F;
	viewport.maxDepth = 1.0F;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapExtent;
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, 0, 0);
}


void Renderer::UpdateUniforms(const UniformBufferObject& ubo){
    memcpy(m_uniformBufferMapped[m_currentFrame], &ubo, sizeof(ubo));
}

void Renderer::BeginUI(){
    // ImGui frame is started in UIManager::BeginFrame()
}

void Renderer::EndUI(){
    ImDrawData* drawData = ImGui::GetDrawData();
    if(drawData){
        ImGui_ImplVulkan_RenderDrawData(drawData, m_commandBuffers[m_currentFrame]);
    }
}
