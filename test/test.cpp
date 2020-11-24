#define VMA_IMPLEMENTATION

#include <vkf/swapchain.hpp>

// 
void error(int errnum, const char* errmsg)
{
    std::cerr << errnum << ": " << errmsg << std::endl;
};

// 
const uint32_t SCR_WIDTH = 800u, SCR_HEIGHT = 600u;

// 
int main() {
    glfwSetErrorCallback(error);
    glfwInit();

    // 
    if (GLFW_FALSE == glfwVulkanSupported()) {
        glfwTerminate(); return -1;
    };

    // 
    uint32_t canvasWidth = SCR_WIDTH, canvasHeight = SCR_HEIGHT; // For 3840x2160 Resolutions
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // 
    float xscale = 1.f, yscale = 1.f;
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    glfwGetMonitorContentScale(primary, &xscale, &yscale);

    // 
    std::string title = "TestApp";
    vkt::uni_ptr<vkf::Instance> instance = std::make_shared<vkf::Instance>();
    vkt::uni_ptr<vkf::Device> device = std::make_shared<vkf::Device>(instance);
    vkt::uni_ptr<vkf::Queue> queue = std::make_shared<vkf::Queue>(device);
    vkt::uni_ptr<vkf::SwapChain> manager = std::make_shared<vkf::SwapChain>(device);

    // 
    instance->create();
    vkf::SurfaceWindow& surface = manager->createWindowSurface(SCR_WIDTH * xscale, SCR_HEIGHT * yscale, title);

    // 
    device->create(0u, surface.surface);
    queue->create();

    // 
    vkf::SurfaceFormat& format = manager->getSurfaceFormat();
    VkRenderPass& renderPass = manager->createRenderPass();
    VkSwapchainKHR& swapchain = manager->createSwapchain();
    std::vector<vkf::Framebuffer>& framebuffers = manager->createSwapchainFramebuffer(queue);

    // 
    auto allocInfo = vkt::MemoryAllocationInfo{};
    allocInfo.device = *device;
    allocInfo.memoryProperties = device->memoryProperties;
    allocInfo.instanceDispatch = instance->dispatch;
    allocInfo.deviceDispatch = device->dispatch;

    // 
    auto renderArea = vkh::VkRect2D{ vkh::VkOffset2D{0, 0}, vkh::VkExtent2D{ uint32_t(canvasWidth / 1.f * xscale), uint32_t(canvasHeight / 1.f * yscale) } };
    auto viewport = vkh::VkViewport{ 0.0f, 0.0f, static_cast<float>(renderArea.extent.width), static_cast<float>(renderArea.extent.height), 0.f, 1.f };


    // TODO: Inline Uniforms
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
    auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
    auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };
    std::vector<VkDescriptorSetLayout> layouts = {};
    std::vector<vkh::VkPushConstantRange> ranges = { vkh::VkPushConstantRange{.stageFlags = pipusage, .offset = 0u, .size = 16u } };
    vkh::handleVk(device->dispatch->CreatePipelineLayout(vkh::VkPipelineLayoutCreateInfo{  }.setSetLayouts(layouts).setPushConstantRanges(ranges), nullptr, &pipelineLayout));

    // TODO: Descriptor Sets
    std::vector<VkDescriptorSet> descriptorSets = {};


    // 
    vkh::VsGraphicsPipelineCreateInfoConstruction pipelineInfo = {};
    pipelineInfo.stages = {
        vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(std::string("./shaders/render.vert.spv")), VK_SHADER_STAGE_VERTEX_BIT),
        vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(std::string("./shaders/render.frag.spv")), VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    pipelineInfo.graphicsPipelineCreateInfo.layout = pipelineLayout;
    pipelineInfo.graphicsPipelineCreateInfo.renderPass = renderPass;//context->refRenderPass();
    pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineInfo.viewportState.pViewports = &reinterpret_cast<::VkViewport&>(viewport);
    pipelineInfo.viewportState.pScissors = &reinterpret_cast<::VkRect2D&>(renderArea);
    pipelineInfo.colorBlendAttachmentStates = { {} }; // Default Blend State
    pipelineInfo.dynamicStates = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

    // 
    VkPipeline finalPipeline = {};
    vkh::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &finalPipeline));


#define ENABLE_RAY_TRACING
#ifdef ENABLE_RAY_TRACING
    //
    std::vector<uint16_t> indices = { 0, 1, 2 };
    std::vector<glm::vec4> vertices = {
        glm::vec4( 1.f, -1.f, -1.f, 1.f),
        glm::vec4(-1.f, -1.f, -1.f, 1.f),
        glm::vec4( 0.f,  1.f, -1.f, 1.f)
    };
    std::vector<uint32_t> primitiveCounts = { 1u };
    std::vector<uint32_t> instanceCounts = { 1u };

    //
    vkt::Vector<uint16_t> indicesBuffer = {};
    vkt::Vector<glm::vec4> verticesBuffer = {};

    {   // vertices
        auto size = vertices.size() * sizeof(glm::vec4);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        };
        auto vmaCreateInfo = vkt::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        verticesBuffer = vkt::Vector<glm::vec4>(allocation, 0ull, size, sizeof(glm::vec4));
        memcpy(verticesBuffer.map(), vertices.data(), size);
    }

    {   // indices
        auto size = indices.size() * sizeof(uint16_t);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        };
        auto vmaCreateInfo = vkt::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        indicesBuffer = vkt::Vector<uint16_t>(allocation, 0ull, size, sizeof(uint16_t));
        memcpy(indicesBuffer.map(), indices.data(), size);
    }


    // bottom levels
    vkt::Vector<uint8_t> accelerationStructureBottomScratch = {};
    vkt::Vector<uint8_t> accelerationStructureBottomStorage = {};
    VkAccelerationStructureKHR accelerationStructureBottom = VK_NULL_HANDLE;
    {
        auto accelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        vkh::VkAccelerationStructureGeometryKHR geometryInfo = {
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry = vkh::VkAccelerationStructureGeometryTrianglesDataKHR{
                .vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT,
                .vertexData = verticesBuffer.deviceAddress(),
                .vertexStride = sizeof(glm::vec4),
                .maxVertex = 3u,
                .indexType = VK_INDEX_TYPE_UINT16,
                .indexData = indicesBuffer.deviceAddress(),
                .transformData = 0ull
            },
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
        };
        vkh::VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
            .type = accelerationStructureType,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .geometryCount = 1u,
            .pGeometries = &geometryInfo,
        };

        // get acceleration structure size
        vkh::VkAccelerationStructureBuildSizesInfoKHR sizes = {};
        device->dispatch->GetAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveCounts.data(), &sizes);

        {   // create acceleration structure storage
            auto bufferCreateInfo = vkh::VkBufferCreateInfo{
                .size = sizes.accelerationStructureSize,
                .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            };
            auto vmaCreateInfo = vkt::VmaMemoryInfo{
                .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                .instanceDispatch = instance->dispatch,
                .deviceDispatch = device->dispatch
            };
            auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
            accelerationStructureBottomStorage = vkt::Vector<uint8_t>(allocation, 0ull, sizes.accelerationStructureSize, sizeof(uint8_t));
        };

        {   // create acceleration structure scratch
            auto bufferCreateInfo = vkh::VkBufferCreateInfo{
                .size = sizes.buildScratchSize,
                .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            };
            auto vmaCreateInfo = vkt::VmaMemoryInfo{
                .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                .instanceDispatch = instance->dispatch,
                .deviceDispatch = device->dispatch
            };
            auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
            accelerationStructureBottomScratch = vkt::Vector<uint8_t>(allocation, 0ull, sizes.buildScratchSize, sizeof(uint8_t));
        };

        // create acceleration structure
        vkh::VkAccelerationStructureCreateInfoKHR accelerationInfo = {};
        accelerationInfo.type = accelerationStructureType;
        accelerationInfo = accelerationStructureBottomStorage;

        // currently, not available (driver or loader bug)
        device->dispatch->CreateAccelerationStructureKHR(accelerationInfo, nullptr, &accelerationStructureBottom);

        //
        buildInfo.dstAccelerationStructure = accelerationStructureBottom;
        buildInfo.scratchData = accelerationStructureBottomScratch;

        //
        vkh::VkAccelerationStructureBuildRangeInfoKHR rangesInfo = {
            .primitiveCount = primitiveCounts[0]
        };
        std::vector<::VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPointers = {
            &rangesInfo
        };

        // needs command buffer execution
        queue->submitOnce([&](VkCommandBuffer cmd) {
            device->dispatch->BuildAccelerationStructuresKHR(VK_NULL_HANDLE, 1u, &buildInfo, rangeInfoPointers.data());
        });
    };


    // top, instance levels
    vkh::VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {};
    std::vector<::VkAccelerationStructureInstanceKHR> instances = {
        vkh::VkAccelerationStructureInstanceKHR{
            .accelerationStructureReference = device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = accelerationStructureBottom))
        }
    };
    vkt::Vector<vkh::VkAccelerationStructureInstanceKHR> instancesBuffer = {};
    {   // instances
        auto size = instances.size() * sizeof(vkh::VkAccelerationStructureInstanceKHR);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        };
        auto vmaCreateInfo = vkt::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        instancesBuffer = vkt::Vector<vkh::VkAccelerationStructureInstanceKHR>(allocation, 0ull, size, sizeof(vkh::VkAccelerationStructureInstanceKHR));
        memcpy(instancesBuffer.map(), instances.data(), size);
    };


    // top levels
    vkt::Vector<uint8_t> accelerationStructureTopScratch = {};
    vkt::Vector<uint8_t> accelerationStructureTopStorage = {};
    VkAccelerationStructureKHR accelerationStructureTop = VK_NULL_HANDLE;
    {
        auto accelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        vkh::VkAccelerationStructureGeometryKHR geometryInfo = {
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry = {},
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
        };
        geometryInfo.geometry = vkh::VkAccelerationStructureGeometryInstancesDataKHR{
            .data = instancesBuffer.deviceAddress()
        };
        vkh::VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
            .type = accelerationStructureType,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .geometryCount = 1u,
            .pGeometries = &geometryInfo,
        };

        // get acceleration structure size
        vkh::VkAccelerationStructureBuildSizesInfoKHR sizes = {};
        device->dispatch->GetAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, instanceCounts.data(), &sizes);

        {   // create acceleration structure storage
            auto bufferCreateInfo = vkh::VkBufferCreateInfo{
                .size = sizes.accelerationStructureSize,
                .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            };
            auto vmaCreateInfo = vkt::VmaMemoryInfo{
                .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                .instanceDispatch = instance->dispatch,
                .deviceDispatch = device->dispatch
            };
            auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
            accelerationStructureBottomStorage = vkt::Vector<uint8_t>(allocation, 0ull, sizes.accelerationStructureSize, sizeof(uint8_t));
        };

        {   // create acceleration structure scratch
            auto bufferCreateInfo = vkh::VkBufferCreateInfo{
                .size = sizes.buildScratchSize,
                .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            };
            auto vmaCreateInfo = vkt::VmaMemoryInfo{
                .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                .instanceDispatch = instance->dispatch,
                .deviceDispatch = device->dispatch
            };
            auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
            accelerationStructureTopScratch = vkt::Vector<uint8_t>(allocation, 0ull, sizes.buildScratchSize, sizeof(uint8_t));
        };

        // create acceleration structure
        vkh::VkAccelerationStructureCreateInfoKHR accelerationInfo = {};
        accelerationInfo.type = accelerationStructureType;
        accelerationInfo = accelerationStructureBottomStorage;

        // currently, not available (driver or loader bug)
        device->dispatch->CreateAccelerationStructureKHR(accelerationInfo, nullptr, &accelerationStructureTop);

        //
        buildInfo.dstAccelerationStructure = accelerationStructureTop;
        buildInfo.scratchData = accelerationStructureTopScratch;

        //
        vkh::VkAccelerationStructureBuildRangeInfoKHR rangesInfo = {
            .primitiveCount = instanceCounts[0]
        };
        std::vector<::VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPointers = {
            &rangesInfo
        };

        // needs command buffer execution
        queue->submitOnce([&](VkCommandBuffer cmd) {
            device->dispatch->BuildAccelerationStructuresKHR(VK_NULL_HANDLE, 1u, &buildInfo, rangeInfoPointers.data());
        });
    };
#endif

    //
    std::vector<uint64_t> addresses = {
        device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = accelerationStructureTop))
    };

    //
    VkPipelineLayout rtPipelineLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> rtLayouts = { VK_NULL_HANDLE };

    // create descriptor set layout
    vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
    descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
        .binding = 0u,
        .descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT,
        .descriptorCount = uint32_t(addresses.size() * sizeof(uint64_t)), // TODO: fix descriptor counting
        .stageFlags = pipusage
    }, vkh::VkDescriptorBindingFlags{});
    vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &rtLayouts[0]));

    // create pipeline layout
    std::vector<vkh::VkPushConstantRange> rtRanges = { vkh::VkPushConstantRange{.stageFlags = pipusage, .offset = 0u, .size = 16u } };
    vkh::handleVk(device->dispatch->CreatePipelineLayout(vkh::VkPipelineLayoutCreateInfo{  }.setSetLayouts(rtLayouts).setPushConstantRanges(rtRanges), nullptr, &rtPipelineLayout));
    std::vector<VkDescriptorSet> rtDescriptorSets = { VK_NULL_HANDLE };

    // create descriptor set
    vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(rtLayouts[0], device->descriptorPool);
    descriptorSetHelper.pushDescription<uint64_t>(vkh::VkDescriptorUpdateTemplateEntry{
        .dstBinding = 0u,
        .descriptorCount = uint32_t(addresses.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT
    }) = addresses[0];
    bool created = false;
    vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, rtDescriptorSets[0], created);



    // 
    int64_t currSemaphore = -1;
    uint32_t currentBuffer = 0u;
    uint32_t frameCount = 0u;

    // 
    while (!glfwWindowShouldClose(surface.window)) { // 
        glfwPollEvents();

        // 
        int64_t n_semaphore = currSemaphore, c_semaphore = (currSemaphore + 1) % framebuffers.size(); // Next Semaphore
        currSemaphore = (c_semaphore = c_semaphore >= 0 ? c_semaphore : int64_t(framebuffers.size()) + c_semaphore); // Current Semaphore
        (n_semaphore = n_semaphore >= 0 ? n_semaphore : int64_t(framebuffers.size()) + n_semaphore); // Fix for Next Semaphores

        // 
        vkh::handleVk(device->dispatch->WaitForFences(1u, &framebuffers[c_semaphore].waitFence, true, 30ull * 1000ull * 1000ull * 1000ull));
        vkh::handleVk(device->dispatch->ResetFences(1u, &framebuffers[c_semaphore].waitFence));
        vkh::handleVk(device->dispatch->AcquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), framebuffers[c_semaphore].presentSemaphore, nullptr, &currentBuffer));
        //fw->getDeviceDispatch()->SignalSemaphore(vkh::VkSemaphoreSignalInfo{.semaphore = framebuffers[n_semaphore].semaphore, .value = 1u});

        // 
        vkh::VkClearValue clearValues[2] = { {}, {} };
        clearValues[0].color = vkh::VkClearColorValue{}; clearValues[0].color.float32 = glm::vec4(0.f, 0.f, 0.f, 0.f);
        clearValues[1].depthStencil = VkClearDepthStencilValue{ 1.0f, 0 };

        // Create render submission 
        std::vector<VkSemaphore> waitSemaphores = { framebuffers[currentBuffer].presentSemaphore }, signalSemaphores = { framebuffers[currentBuffer].drawSemaphore };
        std::vector<VkPipelineStageFlags> waitStages = {
            vkh::VkPipelineStageFlags{.eFragmentShader = 1, .eComputeShader = 1, .eTransfer = 1, .eRayTracingShader = 1, .eAccelerationStructureBuild = 1 },
            vkh::VkPipelineStageFlags{.eFragmentShader = 1, .eComputeShader = 1, .eTransfer = 1, .eRayTracingShader = 1, .eAccelerationStructureBuild = 1 }
        };

        // create command buffer (with rewrite)
        VkCommandBuffer& commandBuffer = framebuffers[currentBuffer].commandBuffer;
        if (!commandBuffer) {
            commandBuffer = vkt::createCommandBuffer(device->dispatch, queue->commandPool, false, false); // do reference of cmd buffer

            // Use as present image
            {
                auto aspect = vkh::VkImageAspectFlags{ .eColor = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = framebuffers[currentBuffer].image,
                    .targetLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .originLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // Reuse depth as general
            {
                auto aspect = vkh::VkImageAspectFlags{ .eDepth = 1u, .eStencil = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = manager->depthImage.getImage(),
                    .targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .originLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // 
            device->dispatch->CmdBeginRenderPass(commandBuffer, vkh::VkRenderPassBeginInfo{ .renderPass = renderPass, .framebuffer = framebuffers[currentBuffer].frameBuffer, .renderArea = renderArea, .clearValueCount = 2u, .pClearValues = reinterpret_cast<vkh::VkClearValue*>(&clearValues[0]) }, VK_SUBPASS_CONTENTS_INLINE);
            device->dispatch->CmdSetViewport(commandBuffer, 0u, 1u, viewport);
            device->dispatch->CmdSetScissor(commandBuffer, 0u, 1u, renderArea);
            device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline);
            device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);
            device->dispatch->CmdDraw(commandBuffer, 4, 1, 0, 0);
            device->dispatch->CmdEndRenderPass(commandBuffer);
            vkt::commandBarrier(device->dispatch, commandBuffer);

            // Use as present image
            {
                auto aspect = vkh::VkImageAspectFlags{ .eColor = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = framebuffers[currentBuffer].image,
                    .targetLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .originLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // Reuse depth as general
            {
                auto aspect = vkh::VkImageAspectFlags{ .eDepth = 1u, .eStencil = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = manager->depthImage.getImage(),
                    .targetLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .originLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // 
            device->dispatch->EndCommandBuffer(commandBuffer);
        };

        // Submit command once
        vkh::handleVk(device->dispatch->QueueSubmit(queue->queue, 1u, vkh::VkSubmitInfo{
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()), .pWaitSemaphores = waitSemaphores.data(), .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1u, .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()), .pSignalSemaphores = signalSemaphores.data()
        }, framebuffers[currentBuffer].waitFence));

        // 
        waitSemaphores = { framebuffers[c_semaphore].drawSemaphore };
        vkh::handleVk(device->dispatch->QueuePresentKHR(queue->queue, vkh::VkPresentInfoKHR{
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()), .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = 1, .pSwapchains = &swapchain,
            .pImageIndices = &currentBuffer, .pResults = nullptr
        }));



    };


    return 0;
};
