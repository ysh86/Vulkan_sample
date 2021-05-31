#include "vulkan/vulkan.hpp"

#include <iomanip>
#include <iostream>

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

// debug
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
    // TODO: impl
    return VK_TRUE;
}

int main(int /*argc*/, char ** /*argv*/) {
    try {
        static char const *appName    = "HelloVK";
        static char const *engineName = "Vulkan.hpp";
        static uint32_t const apiVersion = VK_API_VERSION_1_1;
        std::vector<std::string> const &layers     = {};
        std::vector<std::string> const &extensions = {};

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
        vk::ApplicationInfo applicationInfo(appName, 1, engineName, 1, apiVersion);
#if 1
        // in non-debug mode: just use the InstanceCreateInfo for instance creation
        vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo(
            {{}, &applicationInfo, enabledLayers, enabledExtensions}
        );
        vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo.get<vk::InstanceCreateInfo>());
#else
        // in debug mode: addionally use the debugUtilsMessengerCallback in instance creation!
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        );
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        );
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
            {{}, &applicationInfo, enabledLayers, enabledExtensions},
            {{}, severityFlags, messageTypeFlags, debugUtilsMessengerCallback}
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
        vk::UniqueDebugUtilsMessengerEXT debugUtilsMessenger = instance->createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, debugUtilsMessengerCallback)
        );
#endif

        // ---------------------------
        //  enumerate the physicalDevices
        // ---------------------------
        std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

        int i = 0;
        for (auto const &physicalDevice : physicalDevices) {
            vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

            std::cout << "physicalDevices: [" << i << "]" << std::endl;

            std::cout << "apiVersion: ";
            std::cout << ((properties.apiVersion >> 22) & 0xfff) << '.';  // Major.
            std::cout << ((properties.apiVersion >> 12) & 0x3ff) << '.';  // Minor.
            std::cout << (properties.apiVersion & 0xfff);                 // Patch.
            std::cout << std::endl;

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

            ++i;
        }

        // select a GPU
        if (i <= 0) {
            std::cerr << "enumeratePhysicalDevices: PhysicalDevice is not found." << std::endl;
            exit(1);
        }
        i = 0;
        vk::PhysicalDevice &gpu = physicalDevices.front();
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
        if (i <= 0 || !(queueFamilyProperties[0].queueFlags & vk::QueueFlagBits::eCompute)) {
            std::cerr << "queueFamilyProperties: queue is not found." << std::endl;
            exit(1);
        }
        uint32_t queueFamilyIndex = 0;
        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, queueFamilyIndex, 1, &queuePriority);
        std::vector<char const *> enabledDeviceLayers;
        std::vector<char const *> enabledDeviceExtensions; // TODO: request PerformanceQueryFeaturesKHR, HostQueryResetFeatures
        vk::PhysicalDeviceFeatures const *physicalDeviceFeatures = nullptr;
        vk::StructureChain<vk::DeviceCreateInfo> deviceCreateInfo(
            {{}, deviceQueueCreateInfo, enabledDeviceLayers, enabledDeviceExtensions, physicalDeviceFeatures}
        );
        vk::UniqueDevice device = gpu.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>());

        // ---------------------------
        //  Command pool & buffer
        // ---------------------------
        vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex);
        vk::UniqueCommandPool commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1);
        std::vector<vk::CommandBuffer> commandBuffers = device->allocateCommandBuffers(commandBufferAllocateInfo);
        vk::CommandBuffer &commandBuffer = commandBuffers.front();

        // ---------------------------
        //  Queue
        // ---------------------------
        vk::Queue computeQueue = device->getQueue(queueFamilyIndex, 0);

        std::cout << &commandBuffer << std::endl;
        std::cout << &computeQueue << std::endl;


        // vkQueueSubmit


        // setup_render_pass

        // create_pipeline_cache

        // work group size
        // shared data size
        // setup_descriptor_pool
        // set_layout_bindings
        // VkDescriptorSetLayoutCreateInfo
        // VkPipelineLayoutCreateInfo
        // VkDescriptorSetAllocateInfo
        // VkComputePipelineCreateInfo
        // VkSpecializationInfo

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
