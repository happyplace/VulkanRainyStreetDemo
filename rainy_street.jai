#import "Basic";
#import "Input";
#import "Vulkan";

#if OS == .LINUX {
    #import "SDL";

    SDL_Vulkan_GetInstanceExtensions :: (window: *SDL_Window, pCount: *u32, pNames: **u8) -> SDL_bool #foreign SDL2;
    SDL_Vulkan_CreateSurface :: (window: *SDL_Window, instance: VkInstance, surface: *VkSurfaceKHR) -> SDL_bool #foreign SDL2;
}

VULKAN_DEBUG :: true;

window : *void;

VulkanObjects :: struct {
    instance : VkInstance;
    surface: VkSurfaceKHR;
    physical_device: VkPhysicalDevice;
    graphics_queue_index: u32;
    compute_queue_index: u32;
    transfer_queue_index: u32;
    device : VkDevice;
    #if VULKAN_DEBUG {
        debug_report_callback : VkDebugReportCallbackEXT;
    }
}

main :: () {
    width : s32 = 1280;
    height : s32 = 720;

    if SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0 {
        print("failed to init SDL: %\n", to_string(SDL_GetError()));
        return;
    }
    defer SDL_Quit();

    window = SDL_CreateWindow("Vulkan Rainy Street Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if window == null {
        print("failed to create SDL window: %\n", to_string(SDL_GetError()));
        return;
    }
    defer SDL_DestroyWindow(window);

    vulkan_objects: VulkanObjects;
    success := false;

    defer deinit_vulkan(vulkan_objects);
    success, vulkan_objects = init_vulkan();
    if !success
        return;

    quit := false;
    while !quit {
        event : SDL_Event;
        while SDL_PollEvent(*event) {
            if event.type == {
                case SDL_QUIT;
                    quit = true;
                case SDL_KEYUP;
                    if event.key.keysym.sym == SDLK_ESCAPE quit = true;
            }
        }
    }
}

init_vulkan :: () -> bool, VulkanObjects {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;
    vulkan_objects : VulkanObjects;

    // init_swapchain
    // init_depth_stencil
    // init_render_pass
    // init_frame_buffers
    // init_vma_allocator
    // init_shaderc_compiler

    extensions : [..] *u8;
    #if OS == .LINUX {
        extension_count : u32;
        SDL_Vulkan_GetInstanceExtensions(window, *extension_count, null);
        required_extensions := NewArray(extension_count, *u8);
        defer free(required_extensions.data);
        SDL_Vulkan_GetInstanceExtensions(window, *extension_count, required_extensions.data);

        for required_extensions {
            array_add(*extensions, it);
        }
    }
    else
        #assert(false);

    layers : [..] *u8;

    #if VULKAN_DEBUG {
        array_add(*extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME.data);

        array_add(*layers, "VK_LAYER_KHRONOS_validation");
    }

    extension_property_count : u32 = 0;
    vkEnumerateInstanceExtensionProperties(null, *extension_property_count, null);
    extension_properties := NewArray(extension_property_count, VkExtensionProperties);
    defer free(extension_properties.data);
    vkEnumerateInstanceExtensionProperties(null, *extension_property_count, extension_properties.data);

    strlen :: (s : *u8) -> s64 {  // Return the length of s, a C-style zero-terminated string.
        // If you pass in a pointer that is not zero-terminated,
        // BAD things will happen!
        count: s64 = 0;

        while <<s {
            count += 1;
            s += 1;
        }

        return count;
    }

    for extensions {
        jai_str1 := to_string(it, strlen(it));

        supported := false;
        for k : 0..extension_property_count-1 {
            jai_str2 := to_string(extension_properties[k].extensionName.data);
            if jai_str1 == jai_str2 {
                supported = true;
                break;
            }
        }
        if !supported {
            print("\"%\" extension not supported\n", jai_str1);
            return false, vulkan_objects;
        }
    }

    instance_property_count : u32 = 0;
    vkEnumerateInstanceLayerProperties(*instance_property_count, null);
    instance_properties := NewArray(instance_property_count, VkLayerProperties);
    defer free(instance_properties.data);
    vkEnumerateInstanceLayerProperties(*instance_property_count, instance_properties.data);

    for layers {
        str_layer_name := to_string(it, strlen(it));
        supported := false;
        for k : 0..instance_property_count-1 {
            if str_layer_name == to_string(instance_properties[k].layerName.data) {
                supported = true;
                break;
            }
        }
        if !supported {
            print("\"%\" layer not supported\n", str_layer_name);
            return false, vulkan_objects;
        }
    }

    application_info : VkApplicationInfo;
    application_info.sType = .APPLICATION_INFO;
    application_info.pApplicationName = "Vulkan Rainy Street Demo";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "No Engine";
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    instance_create_info : VkInstanceCreateInfo;
    instance_create_info.sType = .INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = *application_info;
    instance_create_info.enabledLayerCount = xx layers.count;
    instance_create_info.ppEnabledLayerNames = layers.data;
    instance_create_info.enabledExtensionCount = xx extensions.count;
    instance_create_info.ppEnabledExtensionNames = extensions.data;

    result = vkCreateInstance(*instance_create_info, null, *vulkan_objects.instance);
    if result != .SUCCESS {
        print("vkCreateInstance failed: %\n", result);
        return false, vulkan_objects;
    }

    #if VULKAN_DEBUG {
        vkCreateDebugReportCallbackEXT : PFN_vkCreateDebugReportCallbackEXT;
        vkCreateDebugReportCallbackEXT = xx vkGetInstanceProcAddr(vulkan_objects.instance,
            "vkCreateDebugReportCallbackEXT");

        report_callback_create_info: VkDebugReportCallbackCreateInfoEXT;
        report_callback_create_info.flags |= .INFORMATION_BIT_EXT;
        report_callback_create_info.flags |= .WARNING_BIT_EXT;
        report_callback_create_info.flags |= .ERROR_BIT_EXT;
        report_callback_create_info.flags |= .PERFORMANCE_WARNING_BIT_EXT;
        report_callback_create_info.flags |= .DEBUG_BIT_EXT;
        report_callback_create_info.pfnCallback = vulkan_debug_callback;

        result = vkCreateDebugReportCallbackEXT(vulkan_objects.instance, *report_callback_create_info,
            null, *vulkan_objects.debug_report_callback);
        if result != .SUCCESS {
            print("vkCreateDebugReportCallbackEXT failed: %\n", result);
            return false, vulkan_objects;
        }
    }

    #if OS == .LINUX {
        if !SDL_Vulkan_CreateSurface(window, vulkan_objects.instance, *vulkan_objects.surface) {
            print("failed to create SDL surface: %\n", to_string(SDL_GetError()));
        }
    }
    else
        #assert(false);

    physical_device_count : u32 = 0;
    vkEnumeratePhysicalDevices(vulkan_objects.instance, *physical_device_count, null);
    physical_devices := NewArray(physical_device_count, VkPhysicalDevice);
    defer free(physical_devices.data);
    vkEnumeratePhysicalDevices(vulkan_objects.instance, *physical_device_count, physical_devices.data);

    selected_index : s64 = -1;
    selected_device_type : VkPhysicalDeviceType = .RANGE_SIZE;
    selected_graphics_queue : s64 = -1;
    selected_compute_queue : s64 = -1;
    selected_transfer_queue : s64 = -1;

    for it, index : physical_devices {
        using device_properties : VkPhysicalDeviceProperties;
        vkGetPhysicalDeviceProperties(it, *device_properties);

        queue_family_property_count : u32;
        vkGetPhysicalDeviceQueueFamilyProperties(it, *queue_family_property_count, null);
        queue_family_properties := NewArray(queue_family_property_count, VkQueueFamilyProperties);
        defer free(queue_family_properties.data);
        vkGetPhysicalDeviceQueueFamilyProperties(it, *queue_family_property_count, queue_family_properties.data);

        graphics_index : s32 = -1;
        compute_index : s32 = -1;
        queues_supporting_transfer : [..] s32;

        for family_property, index : queue_family_properties {
            supports_present : u32;
            vkGetPhysicalDeviceSurfaceSupportKHR(it, xx index, vulkan_objects.surface, *supports_present);

            if graphics_index <= -1 && (family_property.queueFlags & .GRAPHICS_BIT) && supports_present
                graphics_index = xx index;

            if compute_index <= -1 && (family_property.queueFlags & .COMPUTE_BIT)
                compute_index = xx index;

            if (family_property.queueFlags & .TRANSFER_BIT)
                array_add(*queues_supporting_transfer, xx index);
        }

        if graphics_index >= 0 && compute_index >= 0 {
            if deviceType == .DISCRETE_GPU || deviceType < selected_device_type {
                selected_index = index;
                selected_device_type = deviceType;
                selected_graphics_queue = graphics_index;
                selected_compute_queue = compute_index;

                selected_transfer_queue = -1;
                for potential_transfer_index : queues_supporting_transfer {
                    if potential_transfer_index != graphics_index && potential_transfer_index != compute_index {
                        selected_transfer_queue = potential_transfer_index;
                        break;
                    }
                }
                if selected_transfer_queue <= -1 {
                    print("WARNING: device has no separate queue for transfer\n");
                    if queues_supporting_transfer.count <= 0 {
                        print("failed to find a transfer queue for selected device\n");
                        return false, vulkan_objects;
                    }
                    selected_transfer_queue = queues_supporting_transfer[0];
                }
            }
        }
    }

    if selected_index <= -1 {
        print("no GPU with support for graphics and compute was found\n");
        return false, vulkan_objects;
    }

    vulkan_objects.physical_device = physical_devices[selected_index];
    vulkan_objects.graphics_queue_index = xx selected_graphics_queue;
    vulkan_objects.compute_queue_index = xx selected_compute_queue;
    vulkan_objects.transfer_queue_index = xx selected_transfer_queue;

    device_extension_count : u32;
    vkEnumerateDeviceExtensionProperties(vulkan_objects.physical_device, null, *device_extension_count, null);
    device_extensions := NewArray(device_extension_count, VkExtensionProperties);
    defer free(device_extensions.data);
    vkEnumerateDeviceExtensionProperties(vulkan_objects.physical_device, null, *device_extension_count, device_extensions.data);

    enabled_extension_names : [1] *u8;
    enabled_extension_names[0] = "VK_KHR_swapchain";

    for extension_name : enabled_extension_names {
        jai_extension_name := to_string(extension_name);
        extension_supported := false;
        for device_extensions {
            if jai_extension_name == to_string(it.extensionName.data) {
                extension_supported = true;
                break;
            }
        }
        if !extension_supported {
            print("selected device does not support extension '%'\n", jai_extension_name);
            return false, vulkan_objects;
        }
    }

    queue_priority := 1.;

    queue_create_infos : [..] VkDeviceQueueCreateInfo;

    {
        queue_create_info : VkDeviceQueueCreateInfo;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = *queue_priority;
        queue_create_info.queueFamilyIndex = vulkan_objects.graphics_queue_index;
        array_add(*queue_create_infos, queue_create_info);
    }

    if vulkan_objects.graphics_queue_index != vulkan_objects.compute_queue_index {
        queue_create_info : VkDeviceQueueCreateInfo;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = *queue_priority;
        queue_create_info.queueFamilyIndex = vulkan_objects.compute_queue_index;
        array_add(*queue_create_infos, queue_create_info);
    }

    {
        queue_create_info : VkDeviceQueueCreateInfo;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = *queue_priority;
        queue_create_info.queueFamilyIndex = vulkan_objects.transfer_queue_index;
        array_add(*queue_create_infos, queue_create_info);
    }

    physical_device_features : VkPhysicalDeviceFeatures;
    physical_device_features.fillModeNonSolid = VK_TRUE;
    physical_device_features.samplerAnisotropy  = VK_TRUE;

    device_create_info : VkDeviceCreateInfo;
    device_create_info.queueCreateInfoCount = xx queue_create_infos.count;
    device_create_info.pQueueCreateInfos = queue_create_infos.data;
    device_create_info.enabledExtensionCount = xx enabled_extension_names.count;
    device_create_info.ppEnabledExtensionNames = enabled_extension_names.data;
    device_create_info.pEnabledFeatures = *physical_device_features;

    result = vkCreateDevice(vulkan_objects.physical_device, *device_create_info, null, *vulkan_objects.device);
    if result != .SUCCESS {
        print("vkCreateDevice failed: %\n", result);
        return false, vulkan_objects;
    }

    surface_format_count : u32;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_objects.physical_device, vulkan_objects.surface, 
        *surface_format_count, null);
    surface_formats := NewArray(surface_format_count, VkSurfaceFormatKHR);
    defer free(surface_formats.data);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_objects.physical_device, vulkan_objects.surface,
        *surface_format_count, surface_formats.data);

    preferred_surface_formats : [3] VkFormat;
    preferred_surface_formats[0] = .R8G8B8A8_SRGB;
    preferred_surface_formats[1] = .B8G8R8A8_SRGB;
    preferred_surface_formats[2] = .A8B8G8R8_SRGB_PACK32;

    return true, vulkan_objects;
}

deinit_vulkan :: (vulkan_objects: VulkanObjects) {
    #if VULKAN_DEBUG {
        if vulkan_objects.debug_report_callback {
            vkDestroyDebugReportCallbackEXT : PFN_vkDestroyDebugReportCallbackEXT;
            vkDestroyDebugReportCallbackEXT = xx vkGetInstanceProcAddr(vulkan_objects.instance,
                "vkDestroyDebugReportCallbackEXT");

            vkDestroyDebugReportCallbackEXT(vulkan_objects.instance, vulkan_objects.debug_report_callback, null);
        }
    }

    if vulkan_objects.device
        vkDestroyDevice(vulkan_objects.device, null);

    if vulkan_objects.surface
        vkDestroySurfaceKHR(vulkan_objects.instance, vulkan_objects.surface, null);

    if vulkan_objects.instance
        vkDestroyInstance(vulkan_objects.instance, null);
}

#if VULKAN_DEBUG {
    vulkan_debug_callback :: (flags : VkDebugReportFlagsEXT, objType : VkDebugReportObjectTypeEXT, obj : u64,
                              location : u64, code : s32, layerPrefix : *u8, msg : *u8,
                              userData : *void) -> VkBool32 #c_call {
        push_context {
            print("VULKAN VALIDATION: %\n", to_string(msg));
        }
        return VK_FALSE;
    }
}
