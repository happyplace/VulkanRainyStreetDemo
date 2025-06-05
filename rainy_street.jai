#import "Basic";
#import "Input";
#import "Vulkan";
#import "Window_Creation";

main :: () {
    width : u32 = 1280;
    height : u32 = 720;

    window := create_window(xx width, xx height, "Vulkan Rainy Street Demo");

    defer deinit_vulkan();
    if !init_vulkan() {
        print("failed to init vulkan\n");
        return;
    }

    quit := false;
    while !quit {
        update_window_events();

        for events_this_frame {
            if it.type == .QUIT then quit = true;
        }
    }
}

instance : VkInstance;

init_vulkan :: () -> bool {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;

    // init_device
    // init_swapchain
    // init_depth_stencil
    // init_render_pass
    // init_frame_buffers
    // init_vma_allocator
    // init_shaderc_compiler

    extensions: [2] *u8;
    extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME.data;
    extensions[1] = "VK_KHR_xcb_surface";

    layers: [..] *u8;

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
    instance_create_info.enabledExtensionCount = extensions.count;
    instance_create_info.ppEnabledExtensionNames = extensions.data;

    result = vkCreateInstance(*instance_create_info, null, *instance);
    if result != .SUCCESS {
        return false;
    }

    return true;
}

deinit_vulkan :: () {
    vkDestroyInstance(instance, null);
}

