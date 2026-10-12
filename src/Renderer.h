#pragma once
/**
    @file Renderer.h
    @brief Vulkan rendering backend using vk-bootstrap.
*/

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 engineState; // x = RPM, y = Temperature, z = Pressure, w = throttle
    glm::vec4 viewFlags;
};

class Renderer{
    public:
        Renderer() = default;
        ~Renderer();

        bool Initialize(SDL_Window* window, uint32_t width, uint32_t height);
        void Shutdown();

        // Returns false if the frame must be skipped (e.g. swapchain recreation)
        bool BeginFrame();
        void EndFrame();

        // Recreate swapchain after a window resize (no-op if size unchanged)
        bool RecreateSwapchain(uint32_t width, uint32_t height);

        // Upload geometry to GPU (grows internal buffers as needed)
        void UploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        void DrawMesh(uint32_t indexCount, uint32_t instanceCount = 1);

        // Upload the transformation & state uniform buffer
        void UpdateUniforms(const UniformBufferObject& ubo);

        // ImGui integration
        void BeginUI();
        void EndUI();

        // Accessors
        VkDevice GetDevice() const {return m_device;}
        VkInstance GetInstance() const {return m_instance;}
        VkPhysicalDevice GetPhysicalDevice() const {return m_physicalDevice;}
        VkQueue GetGraphicsQueue() const {return m_graphicsQueue;}
        uint32_t GetGraphicsQueueFamily() const {return m_graphicsQueueFamily;}
        VkDescriptorSetLayout GetDescriptorLayout() const {return m_descriptorSetLayout;}
        VkDescriptorPool GetDescriptorPool() const {return m_descriptorPool;}
        VkRenderPass GetRenderPass() const {return m_renderPass;}
        VkExtent2D GetSwapExtent() const {return m_swapExtent;}
        uint32_t GetSwapchainImageCount() const {return static_cast<uint32_t>(m_swapImages.size());}
        VkCommandBuffer GetCurrentCommandBuffer() const {return m_commandBuffers[m_currentFrame];}
        uint32_t GetCurrentFrame() const {return m_currentFrame;}
    private:
        bool CreateInstance(SDL_Window* window);
        bool CreateSurface(SDL_Window* window);
        bool CreateDevice();
        bool CreateSwapchain();
        bool CreateDepthResources();
        void DestroyDepthResources();
        bool CreateRenderPass();
        bool CreateFramebuffers();
        bool CreateCommandPool();
        bool CreateCommandBuffers();
        bool CreateSyncObjects();
        bool CreateDescriptorPool();
        bool CreatePipelineLayout();
        bool CreateGraphicsPipeline();
        bool CreateUniformBuffers();
        bool CreateDescriptorSets();
        void RecreateSwapchainInternal();

        //-------Vulkan Core Objects (managed by vk-bootstrap where possible)
        vkb::Instance m_vkbInstance;
        vkb::Device m_vkbDevice;
        vkb::Swapchain m_vkbSwapchain;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        uint32_t m_graphicsQueueFamily = 0;

        //------Swapchain & Presentation -------------
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_swapImages;
        std::vector<VkImageView> m_swapImageViews;
        VkExtent2D m_swapExtent = {1280, 720};
        VkFormat m_swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
        std::vector<VkFramebuffer> m_framebuffers;

        //------Depth buffer--------------
        VkImage m_depthImage = VK_NULL_HANDLE;
        VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
        VkImageView m_depthImageView = VK_NULL_HANDLE;

        //-------Pipeline---------
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

        //------Commands & Synchronization--------
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer>  m_commandBuffers;
        std::vector<VkSemaphore> m_imageAvailable;
        std::vector<VkSemaphore> m_renderFinished;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight; // per swapchain image
        uint32_t m_currentFrame = 0;
        uint32_t m_imageIndex = 0;
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        //-----Uniform Buffers--------
        std::vector<VkBuffer> m_uniformBuffers;
        std::vector<VkDeviceMemory> m_uniformBuffersMemory;
        std::vector<void*> m_uniformBufferMapped;
        std::vector<VkDescriptorSet> m_descriptorSets;

        //-----Mesh Buffers (host-visible, persistently mapped, grow on demand)
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
        VkDeviceSize m_vertexBufferCapacity = 0;
        void* m_vertexBufferMapped = nullptr;

        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
        VkDeviceSize m_indexBufferCapacity = 0;
        void* m_indexBufferMapped = nullptr;
        uint32_t m_indexCount = 0;

        uint32_t m_width = 1280;
        uint32_t m_height = 720;
        bool m_initialized = false;
};
