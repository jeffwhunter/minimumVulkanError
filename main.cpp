#include <iostream>
#include <print>

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_raii.hpp>

static char const* AppName = "05_InitSwapchainRAII";
static char const* EngineName = "Vulkan.hpp";

namespace
{
    template <typename TargetType, typename SourceType>
    VULKAN_HPP_INLINE TargetType checked_cast(SourceType value)
    {
        static_assert(sizeof(TargetType) <= sizeof(SourceType), "No need to cast from smaller to larger type!");
        static_assert(std::numeric_limits<SourceType>::is_integer, "Only integer types supported!");
        static_assert(!std::numeric_limits<SourceType>::is_signed, "Only unsigned types supported!");
        static_assert(std::numeric_limits<TargetType>::is_integer, "Only integer types supported!");
        static_assert(!std::numeric_limits<TargetType>::is_signed, "Only unsigned types supported!");
        assert(value <= (std::numeric_limits<TargetType>::max)());
        return static_cast<TargetType>(value);
    }

    struct WindowData
    {
        WindowData(GLFWwindow* wnd, std::string const& name, vk::Extent2D const& extent);
        WindowData(const WindowData&) = delete;
        WindowData(WindowData&& other);
        ~WindowData() noexcept;

        GLFWwindow* handle;
        std::string  name;
        vk::Extent2D extent;
    };

    WindowData::WindowData(GLFWwindow* wnd, std::string const& name, vk::Extent2D const& extent) : handle{wnd}, name{name}, extent{extent} {}

    WindowData::WindowData(WindowData&& other) : handle{}, name{}, extent{}
    {
        std::swap(handle, other.handle);
        std::swap(name, other.name);
        std::swap(extent, other.extent);
    }

    WindowData::~WindowData() noexcept
    {
        glfwDestroyWindow(handle);
    }

    WindowData createWindow(std::string const& windowName, vk::Extent2D const& extent)
    {
        struct glfwContext
        {
            glfwContext()
            {
                glfwInit();
                glfwSetErrorCallback(
                    [](int error, const char* msg)
                    {
                        std::cerr << "glfw: "
                            << "(" << error << ") " << msg << std::endl;
                    });
            }

            ~glfwContext()
            {
                glfwTerminate();
            }
        };

        static auto glfwCtx = glfwContext();
        (void)glfwCtx;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(extent.width, extent.height, windowName.c_str(), nullptr, nullptr);
        return WindowData(window, windowName, extent);
    }


    uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties)
    {
        // get the first index into queueFamiliyProperties which supports graphics
        std::vector<vk::QueueFamilyProperties>::const_iterator graphicsQueueFamilyProperty =
            std::find_if(queueFamilyProperties.begin(),
                queueFamilyProperties.end(),
                [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });
        assert(graphicsQueueFamilyProperty != queueFamilyProperties.end());
        return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    }

    std::vector<std::string> getInstanceExtensions()
    {
        std::vector<std::string> extensions;
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
        extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_METAL_EXT )
        extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_VI_NN )
        extensions.push_back(VK_NN_VI_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
        extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XCB_KHR )
        extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
        extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
        extensions.push_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);
#endif
        return extensions;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* /*pUserData*/)
    {
#if !defined( NDEBUG )
        switch (static_cast<uint32_t>(pCallbackData->messageIdNumber))
        {
        case 0:
            // Validation Warning: Override layer has override paths set to C:/VulkanSDK/<version>/Bin
            return vk::False;
        case 0x822806fa:
            // Validation Warning: vkCreateInstance(): to enable extension VK_EXT_debug_utils, but this extension is intended to support use by applications when
            // debugging and it is strongly recommended that it be otherwise avoided.
            return vk::False;
        case 0xe8d1a9fe:
            // Validation Performance Warning: Using debug builds of the validation layers *will* adversely affect performance.
            return vk::False;
        }
#endif

        std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
            << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
        std::cerr << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
        std::cerr << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
        std::cerr << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
        if (0 < pCallbackData->queueLabelCount)
        {
            std::cerr << std::string("\t") << "Queue Labels:\n";
            for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
            {
                std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
            }
        }
        if (0 < pCallbackData->cmdBufLabelCount)
        {
            std::cerr << std::string("\t") << "CommandBuffer Labels:\n";
            for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
            {
                std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
            }
        }
        if (0 < pCallbackData->objectCount)
        {
            std::cerr << std::string("\t") << "Objects:\n";
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
            {
                std::cerr << std::string("\t\t") << "Object " << i << "\n";
                std::cerr << std::string("\t\t\t") << "objectType   = " << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
                    << "\n";
                std::cerr << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
                if (pCallbackData->pObjects[i].pObjectName)
                {
                    std::cerr << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
                }
            }
        }
        return vk::False;
    }

    vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT()
    {
        return {{},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            &debugUtilsMessengerCallback};
    }

#if defined( NDEBUG )
    vk::StructureChain<vk::InstanceCreateInfo>
#else
    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
        makeInstanceCreateInfoChain(vk::InstanceCreateFlagBits        instanceCreateFlagBits,
            vk::ApplicationInfo const& applicationInfo,
            std::vector<char const*> const& layers,
            std::vector<char const*> const& extensions)
    {
#if defined( NDEBUG )
        // in non-debug mode just use the InstanceCreateInfo for instance creation
        vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo({instanceCreateFlagBits, &applicationInfo, layers, extensions});
#else
        // in debug mode, addionally use the debugUtilsMessengerCallback in instance creation!
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
            {instanceCreateFlagBits, &applicationInfo, layers, extensions}, {{}, severityFlags, messageTypeFlags, &debugUtilsMessengerCallback});
#endif
        return instanceCreateInfo;
    }


    std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions
#if !defined( NDEBUG )
        ,
        std::vector<vk::ExtensionProperties> const& extensionProperties
#endif
    )
    {
        std::vector<char const*> enabledExtensions;
        enabledExtensions.reserve(extensions.size());
        for (auto const& ext : extensions)
        {
            assert(std::any_of(
                extensionProperties.begin(), extensionProperties.end(), [ext](vk::ExtensionProperties const& ep) { return ext == ep.extensionName; }));
            enabledExtensions.push_back(ext.data());
        }
#if !defined( NDEBUG )
        if (std::none_of(
            extensions.begin(), extensions.end(), [](std::string const& extension) { return extension == VK_EXT_DEBUG_UTILS_EXTENSION_NAME; }) &&
            std::any_of(extensionProperties.begin(),
                extensionProperties.end(),
                [](vk::ExtensionProperties const& ep) { return (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) == 0); }))
        {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif
        return enabledExtensions;
    }

    std::vector<char const*> gatherLayers(std::vector<std::string> const& layers
#if !defined( NDEBUG )
        ,
        std::vector<vk::LayerProperties> const& layerProperties
#endif
    )
    {
        std::vector<char const*> enabledLayers;
        enabledLayers.reserve(layers.size());
        for (auto const& layer : layers)
        {
            assert(std::any_of(layerProperties.begin(), layerProperties.end(), [layer](vk::LayerProperties const& lp) { return layer == lp.layerName; }));
            enabledLayers.push_back(layer.data());
        }
#if !defined( NDEBUG )
        // Enable standard validation layer to find as much errors as possible!
        if (std::none_of(layers.begin(), layers.end(), [](std::string const& layer) { return layer == "VK_LAYER_KHRONOS_validation"; }) &&
            std::any_of(layerProperties.begin(),
                layerProperties.end(),
                [](vk::LayerProperties const& lp) { return (strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0); }))
        {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
#endif
        return enabledLayers;
    }

    vk::raii::Instance makeInstance(vk::raii::Context const& context,
        std::string const& appName,
        std::string const& engineName,
        std::vector<std::string> const& layers = {},
        std::vector<std::string> const& extensions = {},
        uint32_t                         apiVersion = VK_API_VERSION_1_0)
    {
        vk::ApplicationInfo       applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, apiVersion);
        std::vector<char const*> enabledLayers = gatherLayers(layers
#if !defined( NDEBUG )
            ,
            context.enumerateInstanceLayerProperties()
#endif
        );
        std::vector<char const*> enabledExtensions = gatherExtensions(extensions
#if !defined( NDEBUG )
            ,
            context.enumerateInstanceExtensionProperties()
#endif
        );
#if defined( NDEBUG )
        vk::StructureChain<vk::InstanceCreateInfo>
#else
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
            instanceCreateInfoChain = makeInstanceCreateInfoChain({}, applicationInfo, enabledLayers, enabledExtensions);

        return vk::raii::Instance(context, instanceCreateInfoChain.get<vk::InstanceCreateInfo>());
    }
}

int main()
{
    try
    {
        vk::raii::Context  context;
        vk::raii::Instance instance = makeInstance(context, AppName, EngineName, {}, getInstanceExtensions());
#if !defined( NDEBUG )
        vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger(instance, makeDebugUtilsMessengerCreateInfoEXT());
#endif
        vk::raii::PhysicalDevice physicalDevice = vk::raii::PhysicalDevices(instance).front();

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        uint32_t                               graphicsQueueFamilyIndex = findGraphicsQueueFamilyIndex(queueFamilyProperties);

        uint32_t           width = 64;
        uint32_t           height = 64;
        WindowData window = createWindow(AppName, {width, height});
        VkSurfaceKHR       _surface;

        // This warns 'glfw: (65542) Win32: Vulkan instance missing VK_KHR_win32_surface extension'
        auto result{glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window.handle, nullptr, &_surface)};
        //_surface = 0x0000000000000000 <NULL>

        vk::raii::SurfaceKHR surface(instance, _surface);
        //surface = {m_instance={m_instance=0x000001c5c0830080 {...} } m_surface={m_surfaceKHR=0x0000000000000000 <NULL> } ...}

        //terminates here
        uint32_t presentQueueFamilyIndex = physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface)
            ? graphicsQueueFamilyIndex
            : checked_cast<uint32_t>(queueFamilyProperties.size());

        std::println(std::cerr, "none of the print statements are executed");
    }
    catch (vk::SystemError& err)
    {
        std::println(std::cerr, "vk::SystemError: {}", err.what());
        exit(-1);
    }
    catch (std::exception& err)
    {
        std::println(std::cerr, "std::exception: {}", err.what());
        exit(-1);
    }
    catch (...)
    {
        std::println(std::cerr, "unknown error");
        exit(-1);
    }
    return 0;
}