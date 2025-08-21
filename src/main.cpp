#include "vulkan/vulkan.hpp"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <bitset>
#include <limits>

#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define NDEBUG 1
#define DEBUG (!NDEBUG)

constexpr const uint32_t GPU = 0;
//constexpr const int NUM_OF_DISPATCH = 256;
constexpr const int NUM_OF_DISPATCH = 16;

typedef struct {
    uint32_t r;//, g, b, a;
} pixel_t;
constexpr const int W = 1024;
constexpr const int H = 1024 * 2;
constexpr const int WORKGROUP_SIZE = 256;

constexpr const char *kernelFile = "build/src/hello.comp.spv";
constexpr const char *kernelName = "main";
//constexpr const char *resultFile = "result.png"; // for debug


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
        constexpr uint32_t const appVersion    = VK_MAKE_VERSION(0, 0, 2);
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

        uint32_t i = 0;
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

            // Limits
            // compute
            uint32_t maxComputeWorkGroupCount_x = properties.limits.maxComputeWorkGroupCount.at(0);
            uint32_t maxComputeWorkGroupCount_y = properties.limits.maxComputeWorkGroupCount.at(1);
            uint32_t maxComputeWorkGroupCount_z = properties.limits.maxComputeWorkGroupCount.at(2);
            uint32_t maxComputeWorkGroupSize_x = properties.limits.maxComputeWorkGroupSize.at(0);
            uint32_t maxComputeWorkGroupSize_y = properties.limits.maxComputeWorkGroupSize.at(1);
            uint32_t maxComputeWorkGroupSize_z = properties.limits.maxComputeWorkGroupSize.at(2);
            std::cout << "maxComputeSharedMemorySize: " << properties.limits.maxComputeSharedMemorySize << std::endl;
            std::cout << "maxComputeWorkGroupCount: " << maxComputeWorkGroupCount_x << "," << maxComputeWorkGroupCount_y << "," << maxComputeWorkGroupCount_z << std::endl;
            std::cout << "maxComputeWorkGroupInvocations: " << properties.limits.maxComputeWorkGroupInvocations << std::endl;
            std::cout << "maxComputeWorkGroupSize: " << maxComputeWorkGroupSize_x << "," << maxComputeWorkGroupSize_y << "," << maxComputeWorkGroupSize_z << std::endl;
            // profiling
            std::cout << "timestampComputeAndGraphics: " << properties.limits.timestampComputeAndGraphics << std::endl;
            std::cout << "timestampPeriod: " << properties.limits.timestampPeriod << std::endl;

            std::cout << std::endl;

            // Features
            //vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();

            ++i;
        }

        // select a GPU
        if (i <= GPU) {
            std::cerr << "enumeratePhysicalDevices: PhysicalDevice is not found." << std::endl;
            exit(1);
        }
        vk::PhysicalDevice &gpu = physicalDevices[GPU];
        vk::PhysicalDeviceProperties properties = gpu.getProperties();
        // device extensions
        std::vector<vk::ExtensionProperties> deviceExtensionProperties = gpu.enumerateDeviceExtensionProperties();
        std::cout << "PhysicalDevice " << GPU << " : " << deviceExtensionProperties.size() << " extensions:" << std::endl;
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
            std::cout << "timestampValidBits: " << qfp.timestampValidBits << std::endl;
            std::cout << std::endl;

            ++i;
        }
        // select a queue
        uint32_t queueFamilyIndex = 0;
        if (i <= queueFamilyIndex || !(queueFamilyProperties[queueFamilyIndex].queueFlags & vk::QueueFlagBits::eCompute)) {
            std::cerr << "queueFamilyProperties: compute queue is not found." << std::endl;
            exit(1);
        }
        float queuePriority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos = {
            {{}, queueFamilyIndex, 1, &queuePriority}
        };
        std::vector<char const *> enabledDeviceLayers;
        std::vector<char const *> enabledDeviceExtensions;
        vk::PhysicalDeviceFeatures physicalDeviceFeatures = {};
        vk::StructureChain<vk::DeviceCreateInfo/*, vk::PhysicalDeviceHostQueryResetFeatures*/> deviceCreateInfo(
            {{}, deviceQueueCreateInfos, enabledDeviceLayers, enabledDeviceExtensions, &physicalDeviceFeatures}
            //{true},
            // TODO: PerformanceQueryFeaturesKHR
        );
        vk::UniqueDevice device = gpu.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>());
        //  queue
        vk::Queue computeQueue = device->getQueue(queueFamilyIndex, 0);

        // ---------------------------
        //  Buffers
        // ---------------------------
        constexpr size_t bufferSize = W * H * sizeof(pixel_t);
        vk::BufferCreateInfo bufferCreateInfo({}, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &queueFamilyIndex);
        std::vector<vk::Buffer> buffers = {
            device->createBuffer(bufferCreateInfo),
            device->createBuffer(bufferCreateInfo),
            device->createBuffer(bufferCreateInfo),
        };
        std::vector<vk::Buffer> devBuffers = {
            device->createBuffer(bufferCreateInfo),
            device->createBuffer(bufferCreateInfo),
            device->createBuffer(bufferCreateInfo),
        };

        size_t totalRequiredSize = 0;
        uint32_t typeFilter = 0;
        i = 0;
        for (const vk::Buffer &buffer : buffers) {
            vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(buffer);
            std::cout << "buffers[" << i << "]:" << std::endl;
            std::cout << "memoryRequirements.size: " << memoryRequirements.size << std::endl;
            std::cout << "memoryRequirements.alignment: " << memoryRequirements.alignment << std::endl;
            std::cout << "memoryRequirements.memoryTypeBits: " << std::bitset<32>(memoryRequirements.memoryTypeBits) << std::dec << std::endl;

            totalRequiredSize += memoryRequirements.size;
            typeFilter |= memoryRequirements.memoryTypeBits;
            ++i;
        }
        size_t devTotalRequiredSize = 0;
        uint32_t devTypeFilter = 0;
        i = 0;
        for (const vk::Buffer &buffer : devBuffers) {
            vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(buffer);
            std::cout << "devBuffers[" << i << "]:" << std::endl;
            std::cout << "memoryRequirements.size: " << memoryRequirements.size << std::endl;
            std::cout << "memoryRequirements.alignment: " << memoryRequirements.alignment << std::endl;
            std::cout << "memoryRequirements.memoryTypeBits: " << std::bitset<32>(memoryRequirements.memoryTypeBits) << std::dec << std::endl;

            devTotalRequiredSize += memoryRequirements.size;
            devTypeFilter |= memoryRequirements.memoryTypeBits;
            ++i;
        }
        std::cout << std::endl;

        auto filter = [&](uint32_t typeFilter, vk::Flags<vk::MemoryPropertyFlagBits> properties) {
            for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
                if ((typeFilter & (1 << i)) &&
                   ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
                    std::cout << "select bit" << i << std::endl;
                    return i;
                }
            }
            return 0xffffffffU;
        };

        uint32_t memoryTypeIndex = filter(typeFilter, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        vk::MemoryAllocateInfo memoryAllocateInfo(totalRequiredSize, memoryTypeIndex);
        vk::UniqueDeviceMemory buffersMemory = device->allocateMemoryUnique(memoryAllocateInfo);

        size_t offset = 0;
        for (const vk::Buffer &buffer : buffers) {
            vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(buffer);
            device->bindBufferMemory(buffer, buffersMemory.get(), offset);
            offset += memoryRequirements.size;
        }
        assert(offset == totalRequiredSize);
        std::cout << "total: " << totalRequiredSize << std::endl;

        uint32_t devMemoryTypeIndex = filter(devTypeFilter, vk::MemoryPropertyFlagBits::eDeviceLocal);
        vk::MemoryAllocateInfo devMemoryAllocateInfo(devTotalRequiredSize, devMemoryTypeIndex);
        vk::UniqueDeviceMemory devBuffersMemory = device->allocateMemoryUnique(devMemoryAllocateInfo);

        offset = 0;
        for (const vk::Buffer &buffer : devBuffers) {
            vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(buffer);
            device->bindBufferMemory(buffer, devBuffersMemory.get(), offset);
            offset += memoryRequirements.size;
        }
        assert(offset == devTotalRequiredSize);
        std::cout << "total: " << devTotalRequiredSize << std::endl;
        std::cout << std::endl;

        // ---------------------------
        // Layouts
        // ---------------------------
        std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
            {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}, // src0
            {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}, // src1
            {2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}, // dst
        };
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, descriptorSetLayoutBindings);
        vk::UniqueDescriptorSetLayout descriptorSetLayout = device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

        // ---------------------------
        //  Descriptor Sets
        // ---------------------------
        uint32_t maxSets = 2;
        // Pool
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
            {vk::DescriptorType::eStorageBuffer, static_cast<uint32_t>(descriptorSetLayoutBindings.size())},
            {vk::DescriptorType::eStorageBuffer, static_cast<uint32_t>(descriptorSetLayoutBindings.size())},
        };
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo({}, maxSets, descriptorPoolSizes);
        vk::UniqueDescriptorPool descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
        // Alloc
        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool.get(), descriptorSetLayout.get());
        vk::UniqueDescriptorSet descriptorSet = std::move(
            device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo).front()
        );
        vk::UniqueDescriptorSet devDescriptorSet = std::move(
            device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo).front()
        );
        // bind buffers
        std::vector<vk::DescriptorBufferInfo> descriptorBufferInfos = {
            {buffers[0], 0, VK_WHOLE_SIZE},
            {buffers[1], 0, VK_WHOLE_SIZE},
            {buffers[2], 0, VK_WHOLE_SIZE},
        };
        std::vector<vk::DescriptorBufferInfo> devDescriptorBufferInfos = {
            {devBuffers[0], 0, VK_WHOLE_SIZE},
            {devBuffers[1], 0, VK_WHOLE_SIZE},
            {devBuffers[2], 0, VK_WHOLE_SIZE},
        };
        std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
            {descriptorSet.get(),    0, {}, vk::DescriptorType::eStorageBuffer, {}, descriptorBufferInfos[0], {}},
            {descriptorSet.get(),    1, {}, vk::DescriptorType::eStorageBuffer, {}, descriptorBufferInfos[1], {}},
            {descriptorSet.get(),    2, {}, vk::DescriptorType::eStorageBuffer, {}, descriptorBufferInfos[2], {}},
            {devDescriptorSet.get(), 0, {}, vk::DescriptorType::eStorageBuffer, {}, devDescriptorBufferInfos[0], {}},
            {devDescriptorSet.get(), 1, {}, vk::DescriptorType::eStorageBuffer, {}, devDescriptorBufferInfos[1], {}},
            {devDescriptorSet.get(), 2, {}, vk::DescriptorType::eStorageBuffer, {}, devDescriptorBufferInfos[2], {}},
        };
        device->updateDescriptorSets(writeDescriptorSets, {});

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

        // layout
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, descriptorSetLayout.get());
        vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

        // pipeline
        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eCompute, computeShaderModule.get(), kernelName);
        vk::ComputePipelineCreateInfo computePipelineCreateInfo({}, pipelineShaderStageCreateInfo, pipelineLayout.get());
        vk::ResultValue<vk::UniquePipeline> computePipeline = device->createComputePipelineUnique({}, computePipelineCreateInfo);


        // ---------------------------
        //  Command pool & Command buffer
        // ---------------------------
        // TODO: Which is better?
        //vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex);
        vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, queueFamilyIndex);
        vk::UniqueCommandPool commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1);
        vk::UniqueCommandBuffer commandBuffer = std::move(
            device->allocateCommandBuffersUnique(commandBufferAllocateInfo).front()
        );

        // ---------------------------
        //  Queries
        // ---------------------------
        vk::QueryPoolCreateInfo queryPoolCreateInfo({}, vk::QueryType::eTimestamp, 4, {});
        vk::UniqueQueryPool queryPool = device->createQueryPoolUnique(queryPoolCreateInfo);

        // ---------------------------
        //  exec
        // ---------------------------
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer->begin(beginInfo);
        uint32_t query;
        query = 0;
        {
            //commandBuffer->begin(beginInfo);
            commandBuffer->resetQueryPool(queryPool.get(), query * 2, query * 2 + 2);
            commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool.get(), query * 2);
            {
                commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.value.get());
                commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, 1, &(descriptorSet.get()), 0, nullptr);
                for (int i = 0; i < NUM_OF_DISPATCH; ++i) {
                    commandBuffer->dispatch(W*H/WORKGROUP_SIZE, 1, 1);
                }
            }
            commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool.get(), query * 2 + 1);
            //commandBuffer->end();
        }
        query = 1;
        {
            //commandBuffer->begin(beginInfo);
            commandBuffer->resetQueryPool(queryPool.get(), query * 2, query * 2 + 2);
            commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool.get(), query * 2);
            {
                commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.value.get());
                commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, 1, &(devDescriptorSet.get()), 0, nullptr);
                for (int i = 0; i < NUM_OF_DISPATCH; ++i) {
                    commandBuffer->dispatch(W*H/WORKGROUP_SIZE, 1, 1);
                }
            }
            commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool.get(), query * 2 + 1);
            //commandBuffer->end();
        }
        commandBuffer->end();

        // TODO: prepare src0 & src1

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
        std::cout << std::endl;

        // ---------------------------
        //  save results
        // ---------------------------
        uint64_t timestamps[4];
        result = device->getQueryPoolResults(queryPool.get(), 0, 4, sizeof(uint64_t) * 4, timestamps, sizeof(uint64_t), vk::QueryResultFlagBits::e64);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error(vk::to_string(result));
        }
        //std::cout << "begin: " << timestamps[0] * properties.limits.timestampPeriod << std::endl;
        //std::cout << "end:   " << timestamps[1] * properties.limits.timestampPeriod << std::endl;
        //std::cout << "begin: " << timestamps[2] * properties.limits.timestampPeriod << std::endl;
        //std::cout << "end:   " << timestamps[3] * properties.limits.timestampPeriod << std::endl;
        query = 0;
        {
            uint64_t t0 = timestamps[query * 2];
            uint64_t t1 = timestamps[query * 2 + 1];
            double elapsed_ms;
            if (t1 > t0) {
                elapsed_ms = (t1 - t0) / 1000000.0 * properties.limits.timestampPeriod;
            } else {
                elapsed_ms = (t1 - t0 + std::numeric_limits<uint64_t>::max()) / 1000000.0 * properties.limits.timestampPeriod;
            }
            std::cout << "elapsed time [ms]: " << elapsed_ms << std::endl;
            std::cout << "pinned bandwidth [GB/s]: " << totalRequiredSize * (1000.0 / elapsed_ms) / 1024 / 1024 / 1024 * NUM_OF_DISPATCH << std::endl;
            std::cout << "pinned fma [GFLOPS]: " << (1000.0 / elapsed_ms) * 4/*vec4*/ * 2/*fma*/ * (64*16)/*num*/ * W*H * NUM_OF_DISPATCH / 1000 / 1000 / 1000 << std::endl;
            std::cout << std::endl;
        }
        query = 1;
        {
            uint64_t t0 = timestamps[query * 2];
            uint64_t t1 = timestamps[query * 2 + 1];
            double elapsed_ms;
            if (t1 > t0) {
                elapsed_ms = (t1 - t0) / 1000000.0 * properties.limits.timestampPeriod;
            } else {
                elapsed_ms = (t1 - t0 + std::numeric_limits<uint64_t>::max()) / 1000000.0 * properties.limits.timestampPeriod;
            }
            std::cout << "elapsed time [ms]: " << elapsed_ms << std::endl;
            std::cout << "dev bandwidth [GB/s]: " << totalRequiredSize * (1000.0 / elapsed_ms) / 1024 / 1024 / 1024 * NUM_OF_DISPATCH << std::endl;
            std::cout << "dev fma [GFLOPS]: " << (1000.0 / elapsed_ms) * 4/*vec4*/ * 2/*fma*/ * (64*16)/*num*/ * W*H * NUM_OF_DISPATCH / 1000 / 1000 / 1000 << std::endl;
            std::cout << std::endl;
        }
        // for debug
#if 0
        std::cout << "Save: ";
        {
            pixel_t* mappedMemory = static_cast<pixel_t *>(device->mapMemory(buffersMemory.get(), 0, VK_WHOLE_SIZE));

            size_t offset = W*H * 2; // dst exists after src0 + src1
            std::vector<uint8_t> image;
            image.reserve(W * H * 4);
            for (int i = 0; i < W*H; ++i) {
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[offset + i].r + 0.5f));
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[offset + i].g + 0.5f));
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[offset + i].b + 0.5f));
                image.push_back(static_cast<uint8_t>(255.0f * mappedMemory[offset + i].a + 0.5f));
            }

            device->unmapMemory(buffersMemory.get());
            mappedMemory = nullptr;

            stbi_write_png(resultFile, W, H, 4, image.data(), W*4);
        }
        std::cout << "done" << std::endl;
#endif

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

    std::cout << "done" << std::endl;
    return 0;
}
