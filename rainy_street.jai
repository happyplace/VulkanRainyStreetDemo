#import "Basic";
#import "Input";
#import "Vulkan";
#import "Window_Creation";

VULKAN_DEBUG :: true;

main :: () {
    width : u32 = 1280;
    height : u32 = 720;

    window := create_window(xx width, xx height, "Vulkan Rainy Street Demo");

    defer deinit_vulkan();
    if !init_vulkan()
        return;

    quit := false;
    while !quit {
        update_window_events();

        for events_this_frame {
            if it.type == .QUIT then quit = true;
        }
    }
}

instance : VkInstance;
#if VULKAN_DEBUG {
    debug_report_callback : VkDebugReportCallbackEXT;
}

init_vulkan :: () -> bool {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;

    // init_device
    // init_swapchain
    // init_depth_stencil
    // init_render_pass
    // init_frame_buffers
    // init_vma_allocator
    // init_shaderc_compiler

    extensions: [..] *u8;
    array_add(*extensions, VK_KHR_SURFACE_EXTENSION_NAME.data);
    array_add(*extensions, "VK_KHR_xcb_surface");

    layers: [..] *u8;

    #if VULKAN_DEBUG {
        array_add(*extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME.data);

        array_add(*layers, "VK_LAYER_KHRONOS_validation");
    }

    extension_property_count : u32 = 0;
    vkEnumerateInstanceExtensionProperties(null, *extension_property_count, null);
    extension_properties := NewArray(extension_property_count, VkExtensionProperties);
    defer free(extension_properties.data);
    vkEnumerateInstanceExtensionProperties(null, *extension_property_count, extension_properties.data);

    strlen :: (s: *u8) -> s64 {  // Return the length of s, a C-style zero-terminated string.
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
        for k: 0..extension_property_count-1 {
            jai_str2 := to_string(extension_properties[k].extensionName.data);
            if jai_str1 == jai_str2 {
                supported = true;
                break;
            }
        }
        if !supported {
            print("\"%\" extension not supported\n", jai_str1);
            return false;
        }
    }

    instance_property_count : u32 = 0;
    vkEnumerateInstanceLayerProperties(*instance_property_count, null);
    instance_properties := NewArray(instance_property_count, VkLayerProperties);
    defer free(instance_properties.data);
    vkEnumerateInstanceLayerProperties(*instance_property_count, instance_properties.data);

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

    result = vkCreateInstance(*instance_create_info, null, *instance);
    if result != .SUCCESS {
        print("vkCreateInstance failed: %\n", result);
        return false;
    }

    #if VULKAN_DEBUG {
        vkCreateDebugReportCallbackEXT : PFN_vkCreateDebugReportCallbackEXT;
        vkCreateDebugReportCallbackEXT = xx vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

        report_callback_create_info: VkDebugReportCallbackCreateInfoEXT;
        report_callback_create_info.flags |= .INFORMATION_BIT_EXT;
        report_callback_create_info.flags |= .WARNING_BIT_EXT;
        report_callback_create_info.flags |= .ERROR_BIT_EXT;
        report_callback_create_info.flags |= .PERFORMANCE_WARNING_BIT_EXT;
        report_callback_create_info.flags |= .DEBUG_BIT_EXT;
        report_callback_create_info.pfnCallback = vulkan_debug_callback;

        result = vkCreateDebugReportCallbackEXT(instance, *report_callback_create_info, null, *debug_report_callback);
        if result != .SUCCESS {
            print("vkCreateDebugReportCallbackEXT failed: %\n", result);
            return false;
        }
    }

    return true;
}

deinit_vulkan :: () {
    #if VULKAN_DEBUG {
        if debug_report_callback {
            vkDestroyDebugReportCallbackEXT : PFN_vkDestroyDebugReportCallbackEXT;
            vkDestroyDebugReportCallbackEXT = xx vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

            vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, null);
        }
    }

    if instance
        vkDestroyInstance(instance, null);
}

#if VULKAN_DEBUG {
    vulkan_debug_callback :: (flags: VkDebugReportFlagsEXT, objType: VkDebugReportObjectTypeEXT, obj: u64,
                              location: u64, code: s32, layerPrefix: *u8, msg: *u8,
                              userData: *void) -> VkBool32 #c_call {
        push_context {
            print("VULKAN VALIDATION: %\n", msg);
        }
        return VK_FALSE;
    }
}
