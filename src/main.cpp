#include "vulkan/vulkan.hpp"

#include <iomanip>
#include <iostream>

std::ostream& operator<<(std::ostream &os, vk::ArrayWrapper1D<uint8_t, VK_UUID_SIZE> const &uuid) {
    os << std::setfill('0') << std::hex;
    for (int j = 0; j < VK_UUID_SIZE; ++j) {
        os << std::setw(2) << static_cast<uint32_t>(uuid[j]);
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
        static uint32_t const apiVersion = VK_API_VERSION_1_2;
        std::vector<std::string> const &layers     = {};
        std::vector<std::string> const &extensions = {};

        uint32_t instanceVersion = vk::enumerateInstanceVersion();
        std::cout << "Instance version: " <<
            std::to_string(VK_VERSION_MAJOR(instanceVersion)) << "." <<
            std::to_string(VK_VERSION_MINOR(instanceVersion)) << "." <<
            std::to_string(VK_VERSION_PATCH(instanceVersion)) << std::endl <<
            std::endl;

        // ---------------------------
        //  Layer
        // ---------------------------
        // debug
        std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();

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
        // create a UniqueInstance
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
        // enumerate the physicalDevices
        // ---------------------------
        std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

        for (auto const &physicalDevice : physicalDevices) {
            vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

            std::cout << "apiVersion: ";
            std::cout << ((properties.apiVersion >> 22) & 0xfff) << '.';  // Major.
            std::cout << ((properties.apiVersion >> 12) & 0x3ff) << '.';  // Minor.
            std::cout << (properties.apiVersion & 0xfff);                 // Patch.
            std::cout << std::endl;

            std::cout << "driverVersion: " << properties.driverVersion << std::endl;

            std::cout << std::showbase << std::internal << std::setfill('0') << std::hex;
            std::cout << "vendorId: " << std::setw(6) << properties.vendorID << std::endl;
            std::cout << "deviceId: " << std::setw(6) << properties.deviceID << std::endl;
            std::cout << std::noshowbase << std::right << std::setfill(' ') << std::dec;

            std::cout << "deviceType: " << vk::to_string(properties.deviceType) << std::endl;

            std::cout << "deviceName: " << properties.deviceName << std::endl;

            std::cout << "pipelineCacheUUID: " << properties.pipelineCacheUUID << std::endl;
            std::cout << std::endl;
        }
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
