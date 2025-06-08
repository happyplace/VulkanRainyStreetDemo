#import "Basic";
#import "Input";
#import "Vulkan";
#import "Window_Creation";

#if OS == .LINUX {
    #import "X11";

    libX11 :: #library,system "libX11";

    xcb_connection_t :: struct {}
    xcb_window_t :: struct {}
    VkXcbSurfaceCreateFlagsKHR :: VkFlags;
    VkXcbSurfaceCreateInfoKHR :: struct {
        sType:      VkStructureType = .XCB_SURFACE_CREATE_INFO_KHR;
        pNext:      *void;
        flags:      VkXcbSurfaceCreateFlagsKHR;
        connection: *xcb_connection_t;
        window:     xcb_window_t;
    }

    PFN_vkCreateXcbSurfaceKHR :: #type (instance: VkInstance, pCreateInfo: *VkXcbSurfaceCreateInfoKHR,
        pAllocator: *VkAllocationCallbacks, pSurface: *VkSurfaceKHR) -> VkResult #foreign libvulkan;

    Display :: struct {}
    XGetXCBConnection :: (display: *Display) -> *xcb_connection_t #foreign libX11;
}

VULKAN_DEBUG :: true;

VulkanObjects :: struct {
    instance : VkInstance;
    #if VULKAN_DEBUG {
        debug_report_callback : VkDebugReportCallbackEXT;
    }
}

main :: () {
    width : u32 = 1280;
    height : u32 = 720;

    window := create_window(xx width, xx height, "Vulkan Rainy Street Demo");
    print("%\n", type_of(window));

    vulkan_objects: VulkanObjects;
    success := false;

    defer deinit_vulkan(vulkan_objects);
    success, vulkan_objects = init_vulkan();
    if !success
        return;

    quit := false;
    while !quit {
        update_window_events();

        for events_this_frame {
            if it.type == .QUIT then quit = true;
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
    array_add(*extensions, VK_KHR_SURFACE_EXTENSION_NAME.data);
    #if OS == .LINUX {
        array_add(*extensions, "VK_KHR_xcb_surface");
        // wayland - VK_KHR_wayland_surface
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
        if !x_global_display {
            print("x_global_display is null but the window should be created by now\n");
            return false, vulkan_objects;
        }

        vkCreateXcbSurfaceKHR : PFN_vkCreateXcbSurfaceKHR;
        vkCreateXcbSurfaceKHR = xx vkGetInstanceProcAddr(vulkan_objects.instance,
            "vkCreateXcbSurfaceKHR");

        surface_create_info : VkXcbSurfaceCreateInfoKHR;
        surface_create_info.connection = XGetXCBConnection(x_global_display);
        //surface_create_info.window = xx window;

        /*
            VkXcbSurfaceCreateInfoKHR :: struct {
        sType:      VkStructureType = .XCB_SURFACE_CREATE_INFO_KHR;
        pNext:      *void;
        flags:      VkXcbSurfaceCreateFlagsKHR;
        connection: *xcb_connection_t;
        window:     xcb_window_t;
        }
        */
    }
    else {
        #assert(false);
    }

    physical_device_count : u32 = 0;
    vkEnumeratePhysicalDevices(vulkan_objects.instance, *physical_device_count, null);
    physical_devices := NewArray(physical_device_count, VkPhysicalDevice);
    defer free(physical_devices.data);
    vkEnumeratePhysicalDevices(vulkan_objects.instance, *physical_device_count, physical_devices.data);

    selected_index : s64 = -1;
    selected_device_type : VkPhysicalDeviceType = .RANGE_SIZE;

    for it, index : physical_devices {
        using device_properties : VkPhysicalDeviceProperties;

        vkGetPhysicalDeviceProperties(it, *device_properties);
        if deviceType == .DISCRETE_GPU {
            selected_index = index;
            selected_device_type = .DISCRETE_GPU;
        }
        else {
            if deviceType < selected_device_type {
                selected_device_type = deviceType;
                selected_index = index;
            }
        }

        queue_family_property_count : u32;
        vkGetPhysicalDeviceQueueFamilyProperties(it, *queue_family_property_count, null);
        queue_family_properties := NewArray(queue_family_property_count, VkQueueFamilyProperties);
        defer free(queue_family_properties.data);
        vkGetPhysicalDeviceQueueFamilyProperties(it, *queue_family_property_count, queue_family_properties.data);

        graphics_index : s32 = -1;
        compute_index : s32 = -1;

        /*
        for family_property, index : queue_family_properties {
            supports_present : VkBool32 = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(it, index,
        }
        */
    }

    print("selected %) %\n", selected_index, selected_device_type);

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
