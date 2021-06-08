#include "vulkan/vulkan.hpp"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <bitset>

#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define NDEBUG 1
#define DEBUG (!NDEBUG)

constexpr const char *resultFile = "result.png";
constexpr const char *kernelFile = "src/hello.spv";
constexpr const char *kernelName = "main";
constexpr const int W = 3200;
constexpr const int H = 2400;
constexpr const int WORKGROUP_SIZE = 32;
typedef struct {
    float r, g, b, a;
} pixel_t;

std::ostream& operator<<(std::ostream &os, vk::ArrayWrapper1D<uint8_t, VK_UUID_SIZE> const &uuid) {
    uint8_t const *data = uuid.data();
    os << std::setfill('0') << std::hex;
    for (unsigned j = 0; j < VK_UUID_SIZE; ++j) {
        os << std::setw(2) << static_cast<uint32_t>(data[j]);
        if (j == 3 || j == 5 || j == 7 || j == 9) {
            os << '-';
        }
    }
    os << std::setfill(' ') << std::dec;
    return os;
}

// for debug utils
PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance                                 instance,
    const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
    const VkAllocationCallbacks *              pAllocator,
    VkDebugUtilsMessengerEXT *                 pMessenger) {
    return pfnVkCreateDebugUtilsMessengerEXT( instance, pCreateInfo, pAllocator, pMessenger );
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance                    instance,
    VkDebugUtilsMessengerEXT      messenger,
    VkAllocationCallbacks const * pAllocator) {
    return pfnVkDestroyDebugUtilsMessengerEXT( instance, messenger, pAllocator );
}

VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
    void * /*pUserData*/) {
    std::cerr << "================ Debug ================" << std::endl;
    std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << std::endl;
    std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageTypes)) << std::endl;
    std::cerr << pCallbackData->pMessageIdName << " " << pCallbackData->messageIdNumber << " " << pCallbackData->pMessage << " " << pCallbackData->pNext << std::endl;
    auto pNext = static_cast<VkDebugUtilsMessengerCallbackDataEXT const *>(pCallbackData->pNext);
    while (pNext) {
        std::cerr << pNext->pMessageIdName << " " << pNext->messageIdNumber << " " << pNext->pMessage << " " << pNext->pNext << std::endl;
        pNext = static_cast<VkDebugUtilsMessengerCallbackDataEXT const *>(pNext->pNext);
    }
    std::cerr << "=======================================" << std::endl;

    return VK_TRUE;
}

int main(int /*argc*/, char ** /*argv*/) {
    try {
        constexpr char const *appName          = "HelloVK";
        constexpr uint32_t const appVersion    = VK_MAKE_VERSION(0, 0, 1);
        constexpr char const *engineName       = "Vulkan.hpp";
        constexpr uint32_t const engineVersion = VK_HEADER_VERSION_COMPLETE;
        constexpr uint32_t const apiVersion    = VK_API_VERSION_1_1;

        std::vector<std::string> const layers     = {};
#if !DEBUG
        std::vector<std::string> const extensions = {};
#else
        std::vector<std::string> const extensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
#endif

        uint32_t instanceVersion = vk::enumerateInstanceVersion();
        std::cout << "Instance version: " <<
            VK_VERSION_MAJOR(instanceVersion) << "." <<
            VK_VERSION_MINOR(instanceVersion) << "." <<
            VK_VERSION_PATCH(instanceVersion) << std::endl <<
            std::endl;

        // ---------------------------
        //  Layer
        // ---------------------------
        // debug
        std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
        // instance layers
        std::cout << "Instance" << " : " << layerProperties.size() << " layers:" << std::endl;
        std::sort(layerProperties.begin(), layerProperties.end(),
            [](vk::LayerProperties const &a, vk::LayerProperties const &b) {
                return ::strcmp(a.layerName, b.layerName) < 0;
            }
        );
        for (auto const &lp : layerProperties ) {
            std::cout << "\t" << lp.layerName << ":" << std::endl;
            std::cout << "\t\tVersion: " <<
                VK_VERSION_MAJOR(lp.specVersion) << "." <<
                VK_VERSION_MINOR(lp.specVersion) << "." <<
                VK_VERSION_PATCH(lp.specVersion) << std::endl;
        }
        std::cout << std::endl;

        std::vector<char const *> enabledLayers;
        enabledLayers.reserve(layers.size());
        for (auto const &layer : layers) {
            // debug
            assert(
                std::find_if(layerProperties.begin(), layerProperties.end(), [layer](vk::LayerProperties const &lp) {
                    return layer == lp.layerName;
                }) != layerProperties.end()
            );
            enabledLayers.push_back(layer.data());
        }
        // debug: Enable standard validation layer to find as much errors as possible!
        if (std::find(layers.begin(), layers.end(), "VK_LAYER_KHRONOS_validation") == layers.end() &&
            std::find_if(layerProperties.begin(), layerProperties.end(), [](vk::LayerProperties const &lp) {
                return strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0;
            }) != layerProperties.end()) {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
        if (std::find(layers.begin(), layers.end(), "VK_LAYER_LUNARG_assistant_layer") == layers.end() &&
            std::find_if(layerProperties.begin(), layerProperties.end(), [](vk::LayerProperties const &lp) {
                return strcmp("VK_LAYER_LUNARG_assistant_layer", lp.layerName) == 0;
           }) != layerProperties.end()) {
           enabledLayers.push_back("VK_LAYER_LUNARG_assistant_layer");
        }

        // ---------------------------
        //  Extension
        // ---------------------------
        // debug
        std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();
        // instance extensions
        std::cout << "Instance" << " : " << extensionProperties.size() << " extensions:" << std::endl;
        std::sort(extensionProperties.begin(), extensionProperties.end(),
            [](vk::ExtensionProperties const &a, vk::ExtensionProperties const &b) {
                return ::strcmp(a.extensionName, b.extensionName) < 0;
            }
        );
        for (auto const &ep : extensionProperties ) {
            std::cout << "\t" << ep.extensionName << ":" << std::endl;
            std::cout << "\t\tVersion: " << ep.specVersion << std::endl;
        }
        std::cout << std::endl;

        std::vector<char const *> enabledExtensions;
        enabledExtensions.reserve(extensions.size());
        for (auto const &ext : extensions) {
            // debug
            assert(
                std::find_if(extensionProperties.begin(), extensionProperties.end(), [ext](vk::ExtensionProperties const &ep) {
                    return ext == ep.extensionName;
                }) != extensionProperties.end()
            );
            enabledExtensions.push_back(ext.data());
        }
        // debug
        if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == extensions.end() &&
            std::find_if(extensionProperties.begin(), extensionProperties.end(), [](vk::ExtensionProperties const &ep ) {
                return strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) == 0;
            }) != extensionProperties.end()) {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // ---------------------------
        //  create a UniqueInstance
        // ---------------------------
        vk::ApplicationInfo applicationInfo(appName, appVersion, engineName, engineVersion, apiVersion);
#if !DEBUG
        // in non-debug mode: just use the InstanceCreateInfo for instance creation
        vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo(
            {{}, &applicationInfo, enabledLayers, enabledExtensions}
        );
        vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo.get<vk::InstanceCreateInfo>());
#else
        // in debug mode: addionally use the debugUtilsMessengerCallback in instance creation!
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        );
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        );
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, debugUtilsMessengerCallback);
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
            {{}, &applicationInfo, enabledLayers, enabledExtensions},
            debugUtilsMessengerCreateInfoEXT
        );
        vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo.get<vk::InstanceCreateInfo>());

        pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkCreateDebugUtilsMessengerEXT")
        );
        if (!pfnVkCreateDebugUtilsMessengerEXT) {
            std::cerr << "GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function." << std::endl;
            exit(1);
        }
        pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT")
        );
        if (!pfnVkDestroyDebugUtilsMessengerEXT) {
            std::cerr << "GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function." << std::endl;
            exit(1);
        }
        vk::UniqueDebugUtilsMessengerEXT debugUtilsMessenger = instance->createDebugUtilsMessengerEXTUnique(debugUtilsMessengerCreateInfoEXT);
#endif

        // ---------------------------
        //  enumerate the physicalDevices
        // ---------------------------
        std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

        int i = 0;
        for (auto const &physicalDevice : physicalDevices) {
            vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

            std::cout << "physicalDevices: [" << i << "]" << std::endl;

            // Properties
            std::cout << "apiVersion: ";
            std::cout << VK_VERSION_MAJOR(properties.apiVersion) << '.';
            std::cout << VK_VERSION_MINOR(properties.apiVersion) << '.';
            std::cout << VK_VERSION_PATCH(properties.apiVersion) << std::endl;

            uint32_t ver = properties.driverVersion;
            if (properties.vendorID == 0x10de) {
                // NVIDIA
                std::cout << "driverVersion: " <<
                    ((ver >> 22) & 0x3ff) << "." <<
                    ((ver >> 14) & 0x0ff) << "." <<
                    ((ver >> 6)  & 0x0ff) << std::endl;
            } else {
                std::cout << "driverVersion: " <<
                    VK_VERSION_MAJOR(ver) << "." <<
                    VK_VERSION_MINOR(ver) << "." <<
                    VK_VERSION_PATCH(ver) << std::endl;
            }

            std::cout << std::showbase << std::internal << std::setfill('0') << std::hex;
            std::cout << "vendorId: " << std::setw(6) << properties.vendorID << std::endl;
            std::cout << "deviceId: " << std::setw(6) << properties.deviceID << std::endl;
            std::cout << std::noshowbase << std::right << std::setfill(' ') << std::dec;

            std::cout << "deviceType: " << vk::to_string(properties.deviceType) << std::endl;

            std::cout << "deviceName: " << properties.deviceName << std::endl;

            std::cout << "pipelineCacheUUID: " << properties.pipelineCacheUUID << std::endl;
            std::cout << std::endl;

            // Features
            //vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();

            ++i;
        }

        // select a GPU
        if (i <= 0) {
            std::cerr << "enumeratePhysicalDevices: PhysicalDevice is not found." << std::endl;
            exit(1);
        }
        i = 0;
        vk::PhysicalDevice &gpu = physicalDevices[i];
        // device extensions
        std::vector<vk::ExtensionProperties> deviceExtensionProperties = gpu.enumerateDeviceExtensionProperties();
        std::cout << "PhysicalDevice " << i << " : " << deviceExtensionProperties.size() << " extensions:" << std::endl;
        std::sort(deviceExtensionProperties.begin(), deviceExtensionProperties.end(),
            [](vk::ExtensionProperties const &a, vk::ExtensionProperties const &b) {
                return ::strcmp(a.extensionName, b.extensionName) < 0;
            }
        );
        for (auto const &ep : deviceExtensionProperties ) {
            std::cout << "\t" << ep.extensionName << ":" << std::endl;
            std::cout << "\t\tVersion: " << ep.specVersion << std::endl;
        }
        std::cout << std::endl;

        vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties= gpu.getMemoryProperties();
        std::cout << "physicalDeviceMemoryProperties.memoryHeaps:" << std::endl;
        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryHeapCount; ++i) {
            std::cout << "[" << i << "]: ";
            std::cout << vk::to_string(physicalDeviceMemoryProperties.memoryHeaps[i].flags);
            std::cout << ", " << physicalDeviceMemoryProperties.memoryHeaps[i].size << std::endl;
        }
        std::cout << "physicalDeviceMemoryProperties.memoryTypes:" << std::endl;
        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
            std::cout << "bit" << i << ": ";
            std::cout << vk::to_string(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags);
            std::cout << ", " << "heaps[" << physicalDeviceMemoryProperties.memoryTypes[i].heapIndex << "]" << std::endl;
        }
        std::cout << std::endl;

        // ---------------------------
        //  Create a device
        // ---------------------------
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = gpu.getQueueFamilyProperties();
        i = 0;
        for (auto const &qfp : queueFamilyProperties) {
            std::cout << "queueFamilyProperties: [" << i << "]" << std::endl;
            std::cout << "count: " << qfp.queueCount << std::endl;
            std::cout << "flags: " << vk::to_string(qfp.queueFlags) << std::endl;
            std::cout << std::endl;

            ++i;
        }
        // select a queue
        uint32_t queueFamilyIndex = 0;
        if (i <= 0 || !(queueFamilyProperties[queueFamilyIndex].queueFlags & vk::QueueFlagBits::eCompute)) {
            std::cerr << "queueFamilyProperties: compute queue is not found." << std::endl;
            exit(1);
        }
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, queueFamilyIndex, queuePriority);
        std::vector<char const *> enabledDeviceLayers;
        std::vector<char const *> enabledDeviceExtensions; // TODO: request PerformanceQueryFeaturesKHR, HostQueryResetFeatures
        vk::PhysicalDeviceFeatures physicalDeviceFeatures = {};
        vk::StructureChain<vk::DeviceCreateInfo> deviceCreateInfo(
            {{}, deviceQueueCreateInfo, enabledDeviceLayers, enabledDeviceExtensions, &physicalDeviceFeatures}
        );
        vk::UniqueDevice device = gpu.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>());
        //  queue
        vk::Queue computeQueue = device->getQueue(queueFamilyIndex, 0);

        // ---------------------------
        //  Buffers
        // ---------------------------
        constexpr size_t bufferSize = W * H * sizeof(pixel_t);
        vk::BufferCreateInfo bufferCreateInfo({}, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, queueFamilyIndex);
        vk::UniqueBuffer buffer = device->createBufferUnique(bufferCreateInfo);
        vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(buffer.get());
        auto f = [&](vk::Flags<vk::MemoryPropertyFlagBits> properties) {
            for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
                if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
                   ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
                    std::cout << "select bit: " << i << std::endl;
                    return i;
                }
            }
            return 0xffffffffU;
        };
        vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, f(vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
        vk::UniqueDeviceMemory bufferMemory = device->allocateMemoryUnique(memoryAllocateInfo);
        device->bindBufferMemory(buffer.get(), bufferMemory.get(), 0);

        std::cout << "bufferSize: " << bufferSize << std::endl;
        std::cout << "memoryRequirements.size: " << memoryRequirements.size << std::endl;
        std::cout << "memoryRequirements.alignment: " << memoryRequirements.alignment << std::endl;
        std::cout << "memoryRequirements.memoryTypeBits: " << std::bitset<32>(memoryRequirements.memoryTypeBits) << std::dec << std::endl;

        // ---------------------------
        //  Descriptor Set
        // ---------------------------
        // Layout
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, descriptorSetLayoutBinding);
        vk::UniqueDescriptorSetLayout descriptorSetLayout = device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);
        // Pool
        vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo({}, 1, descriptorPoolSize);
        vk::UniqueDescriptorPool descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
        // Alloc
        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool.get(), descriptorSetLayout.get());
        vk::UniqueDescriptorSet descriptorSet = std::move(
            device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo).front()
        );
        // bind
        vk::DescriptorBufferInfo descriptorBufferInfo(buffer.get(), 0, bufferSize);
        vk::WriteDescriptorSet writeDescriptorSet(descriptorSet.get(), 0, {}, vk::DescriptorType::eStorageBuffer, {}, descriptorBufferInfo, {});
        device->updateDescriptorSets(writeDescriptorSet, {});

        // ---------------------------
        //  Compute pipeline
        // ---------------------------
        // shader
        std::ifstream from(kernelFile, std::ios::in | std::ios::binary);
        std::vector<char> kernelSpv((std::istreambuf_iterator<char>(from)),
                                     std::istreambuf_iterator<char>());
        from.close();
        size_t r = kernelSpv.size() & (sizeof(uint32_t) - 1);
        for (size_t i = 0; i < r; ++i) {
            kernelSpv.push_back(0);
        }
        size_t codeSize = kernelSpv.size();
        uint32_t *pCode = reinterpret_cast<uint32_t *>(kernelSpv.data());
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo({}, codeSize, pCode);
        vk::UniqueShaderModule computeShaderModule = device->createShaderModuleUnique(shaderModuleCreateInfo);

        // pipeline
        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eCompute, computeShaderModule.get(), kernelName);
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, descriptorSetLayout.get());
        vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);
        vk::ComputePipelineCreateInfo computePipelineCreateInfo({}, pipelineShaderStageCreateInfo, pipelineLayout.get());
        vk::UniquePipeline computePipeline = device->createComputePipelineUnique({}, computePipelineCreateInfo);


        // ---------------------------
        //  Command pool & Command buffer
        // ---------------------------
        vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex);
        vk::UniqueCommandPool commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1);
        vk::UniqueCommandBuffer commandBuffer = std::move(
            device->allocateCommandBuffersUnique(commandBufferAllocateInfo).front()
        );

        // ---------------------------
        //  exec
        // ---------------------------
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer->begin(beginInfo);
        {
            commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.get());
            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, 1, &(descriptorSet.get()), 0, nullptr);
            commandBuffer->dispatch(W/WORKGROUP_SIZE, H/WORKGROUP_SIZE, 1);
        }
        commandBuffer->end();

        vk::UniqueFence fence = device->createFenceUnique(vk::FenceCreateInfo());
        vk::SubmitInfo submitInfo({}, {}, commandBuffer.get());

        std::cout << "Run: ";

        computeQueue.submit(submitInfo, fence.get());

        //constexpr uint64_t FenceTimeout100ms = 100000000;
        constexpr uint64_t FenceTimeout100s = 100000000000;
        vk::Result result;
        int timeouts = -1;
        do {
            result = device->waitForFences(fence.get(), true, FenceTimeout100s);
            timeouts++;
        } while (result == vk::Result::eTimeout);
        assert(result == vk::Result::eSuccess);
        if (timeouts != 0) {
            std::cerr << "Unsuitable timeout value, exiting" << std::endl;
            exit(-1);
        }

        std::cout << "done" << std::endl;

        // ---------------------------
        //  save results
        // ---------------------------
        std::cout << "Save: ";
        {
            pixel_t* mappedMemory = static_cast<pixel_t *>(device->mapMemory(bufferMemory.get(), 0, bufferSize));

            std::vector<uint8_t> image;
            image.reserve(W * H * 4);
            for (int i = 0; i < W*H; ++i) {
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[i].r + 0.5f));
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[i].g + 0.5f));
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[i].b + 0.5f));
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[i].a + 0.5f));
            }

            device->unmapMemory(bufferMemory.get());
            mappedMemory = nullptr;

            stbi_write_png(resultFile, W, H, 4, image.data(), W*4);
        }
        std::cout << "done" << std::endl;

        // work group size
        // shared data size

    } catch (vk::SystemError &err) {
        std::cerr << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    } catch (std::exception &err) {
        std::cerr << "std::exception: " << err.what() << std::endl;
        exit(-1);
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        exit(-1);
    }

    std::cout << std::endl << "done" << std::endl;
    return 0;
}
