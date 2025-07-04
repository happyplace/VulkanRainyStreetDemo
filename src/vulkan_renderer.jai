#import "Basic";
#import "Input";
#import "jai-sdl3";
#import "Math";
#import "Vulkan";

libsdl3 :: #library,system "libSDL3";
SDL_Vulkan_GetInstanceExtensions :: (count: *u32) -> **u8 #foreign libsdl3;
SDL_Vulkan_CreateSurface :: (window: *SDL_Window, instance: VkInstance, allocator: *VkAllocationCallbacks, surface: *VkSurfaceKHR) -> [] *u8 #foreign libsdl3;

VULKAN_DEBUG :: true;

VulkanObjects :: struct {
    instance : VkInstance;
    surface: VkSurfaceKHR;
    physical_device: VkPhysicalDevice;
    graphics_queue_index: u32;
    compute_queue_index: u32;
    transfer_queue_index: u32;
    device : VkDevice;
    graphics_queue : VkQueue;
    compute_queue : VkQueue;
    transfer_queue : VkQueue;
    swap_chain_image_count : u32;
    swap_chain : VkSwapchainKHR;
    swap_chain_image_views : [] VkImageView;
    swap_chain_width : u32;
    swap_chain_height : u32;
    swap_chain_format : VkFormat;
    depth_stencil_image : VkImage;
    depth_stencil_image_memory : VkDeviceMemory;
    depth_stencil_image_view : VkImageView;
    render_pass : VkRenderPass;
    framebuffers : [] VkFramebuffer;
    swap_chain_resources : [] VulkanSwapChainResource;
    swap_chain_resource_next_index : u32;
    #if VULKAN_DEBUG {
        debug_report_callback : VkDebugReportCallbackEXT;
    }
}

VulkanSwapChainResource :: struct {
    acquire_image : VkSemaphore;
    release_image : VkSemaphore;
}

VulkanFrameResource :: struct {
    submit_fence : VkFence;
    command_pool : VkCommandPool;
    command_buffer : VkCommandBuffer;
    swap_chain_image_index : u32;
}

init_vulkan :: () -> bool, VulkanObjects {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;
    vulkan_objects : VulkanObjects;

    extensions : [..] *u8;
    #if OS == .LINUX {
        extension_count : u32;
        required_extensions := SDL_Vulkan_GetInstanceExtensions(*extension_count);
        for 0..extension_count-1 {
            array_add(*extensions, required_extensions[it]);
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
        if !SDL_Vulkan_CreateSurface(window, vulkan_objects.instance, null, *vulkan_objects.surface) {
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

    vkGetDeviceQueue(vulkan_objects.device, vulkan_objects.graphics_queue_index, 0, *vulkan_objects.graphics_queue);

    vkGetDeviceQueue(vulkan_objects.device, vulkan_objects.compute_queue_index, 0, *vulkan_objects.compute_queue);

    vkGetDeviceQueue(vulkan_objects.device, vulkan_objects.transfer_queue_index, 0,
        *vulkan_objects.transfer_queue);

    if !init_vulkan_swap_chain(*vulkan_objects)
        return false, vulkan_objects;

    if !init_vulkan_depth_stencil(*vulkan_objects)
        return false, vulkan_objects;

    if !init_vulkan_render_pass(*vulkan_objects)
        return false, vulkan_objects;

    if !init_vulkan_frame_buffers(*vulkan_objects)
        return false, vulkan_objects;

    return true, vulkan_objects;
}

init_vulkan_swap_chain :: (vulkan_objects: *VulkanObjects) -> bool {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;

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

    found_format := false;
    selected_surface_format : VkSurfaceFormatKHR;
    for format : preferred_surface_formats {
        for surface_formats {
            if format == it.format {
                found_format = true;
                selected_surface_format = it;
            }
        }
    }

    if !found_format {
        print("WARNING: no preferred surface format found, using first available format\n");
        selected_surface_format = surface_formats[0];
    }

    surface_capabilities : VkSurfaceCapabilitiesKHR;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_objects.physical_device, vulkan_objects.surface,
        *surface_capabilities);

    swap_chain_size : VkExtent2D;
    if surface_capabilities.currentExtent.width == 0xffffffff {
        window_width : s32;
        window_height : s32;
        SDL_GetWindowSizeInPixels(window, *window_width, *window_height);

        swap_chain_size.width = xx window_width;
        swap_chain_size.height = xx window_height;
    }
    else {
        swap_chain_size = surface_capabilities.currentExtent;
    }

    vulkan_objects.swap_chain_image_count = surface_capabilities.minImageCount;

    pre_transform : VkSurfaceTransformFlagBitsKHR;
    if (surface_capabilities.supportedTransforms & .IDENTITY_BIT_KHR)
        pre_transform = .IDENTITY_BIT_KHR;
    else
        pre_transform = surface_capabilities.currentTransform;

    composite_alpha_flag_bits : VkCompositeAlphaFlagBitsKHR = .OPAQUE_BIT_KHR;
    if (surface_capabilities.supportedCompositeAlpha & .OPAQUE_BIT_KHR)
        composite_alpha_flag_bits = .OPAQUE_BIT_KHR;
    else if (surface_capabilities.supportedCompositeAlpha & .INHERIT_BIT_KHR)
        composite_alpha_flag_bits = .INHERIT_BIT_KHR;
    else if (surface_capabilities.supportedCompositeAlpha & .PRE_MULTIPLIED_BIT_KHR)
        composite_alpha_flag_bits = .PRE_MULTIPLIED_BIT_KHR;
    else if (surface_capabilities.supportedCompositeAlpha & .POST_MULTIPLIED_BIT_KHR)
        composite_alpha_flag_bits = .POST_MULTIPLIED_BIT_KHR;

    prev_swap_chain := vulkan_objects.swap_chain;

    swap_chain_create_info : VkSwapchainCreateInfoKHR;
    swap_chain_create_info.surface = vulkan_objects.surface;
    swap_chain_create_info.minImageCount = vulkan_objects.swap_chain_image_count;
    swap_chain_create_info.imageFormat = selected_surface_format.format;
    swap_chain_create_info.imageColorSpace = selected_surface_format.colorSpace;
    swap_chain_create_info.imageExtent.width = swap_chain_size.width;
    swap_chain_create_info.imageExtent.height = swap_chain_size.height;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = .COLOR_ATTACHMENT_BIT;
    swap_chain_create_info.imageSharingMode = .EXCLUSIVE;
    swap_chain_create_info.preTransform = pre_transform;
    swap_chain_create_info.compositeAlpha = composite_alpha_flag_bits;
    // vsync .FIFO_KHR
    swap_chain_create_info.presentMode = .IMMEDIATE_KHR;
    swap_chain_create_info.oldSwapchain = prev_swap_chain;

    result = vkCreateSwapchainKHR(vulkan_objects.device, *swap_chain_create_info, null, *vulkan_objects.swap_chain);
    if result != .SUCCESS {
        print("vkCreateSwapchainKHR failed: %\n", result);
        return false;
    }

    deinit_vulkan_swap_chain_images(vulkan_objects);

    if prev_swap_chain
        vkDestroySwapchainKHR(vulkan_objects.device, prev_swap_chain, null);

    vulkan_objects.swap_chain_width = swap_chain_size.width;
    vulkan_objects.swap_chain_height = swap_chain_size.height;
    vulkan_objects.swap_chain_format = selected_surface_format.format;

    vkGetSwapchainImagesKHR(vulkan_objects.device, vulkan_objects.swap_chain,
        *vulkan_objects.swap_chain_image_count, null);
    swap_chain_images := NewArray(vulkan_objects.swap_chain_image_count, VkImage);
    defer free(swap_chain_images.data);
    vkGetSwapchainImagesKHR(vulkan_objects.device, vulkan_objects.swap_chain,
        *vulkan_objects.swap_chain_image_count, swap_chain_images.data);

    vulkan_objects.swap_chain_image_views = NewArray(vulkan_objects.swap_chain_image_count, VkImageView);
    for i : 0..vulkan_objects.swap_chain_image_views.count-1 {
        image_view_create_info : VkImageViewCreateInfo;
        image_view_create_info.image = swap_chain_images[i];
        image_view_create_info.viewType = ._2D;
        image_view_create_info.format = vulkan_objects.swap_chain_format;
        image_view_create_info.components.r = .R;
        image_view_create_info.components.g = .G;
        image_view_create_info.components.b = .B;
        image_view_create_info.components.a = .A;
        image_view_create_info.subresourceRange.aspectMask = .COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        result = vkCreateImageView(vulkan_objects.device, *image_view_create_info, null,
            *vulkan_objects.swap_chain_image_views[i]);
        if result != .SUCCESS {
            print("vkCreateImageView failed for swapchain: %\n", result);
            return false;
        }
    }

    vulkan_objects.swap_chain_resources = NewArray(vulkan_objects.swap_chain_image_count, VulkanSwapChainResource);
    for vulkan_objects.swap_chain_resources {
        semaphore_create_info : VkSemaphoreCreateInfo;

        result = vkCreateSemaphore(vulkan_objects.device, *semaphore_create_info, null, *it.acquire_image);
        if result != .SUCCESS {
            print("vkCreateSemaphore swap_chain_resource failed\n");
            return false;
        }

        result = vkCreateSemaphore(vulkan_objects.device, *semaphore_create_info, null, *it.release_image);
        if result != .SUCCESS {
            print("vkCreateSemaphore swap_chain_resource failed\n");
            return false;
        }
    }

    return true;
}

vulkan_find_memory_by_flag_and_type :: (vulkan_objects : VulkanObjects,
                                        memory_property_flag_bits : VkMemoryPropertyFlagBits,
                                        memory_type_bits : u32) -> bool, u32 {
    memory_properties : VkPhysicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkan_objects.physical_device, *memory_properties);

    for i : 0..memory_properties.memoryTypeCount-1 {
        if (memory_type_bits & (1<<i)) != 0 {
            if (memory_properties.memoryTypes[i].propertyFlags & memory_property_flag_bits) != 0
                return true, i;
        }
    }

    return false, 0;
}

init_vulkan_depth_stencil :: (vulkan_objects : *VulkanObjects) -> bool {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;

    image_create_info : VkImageCreateInfo;
    image_create_info.imageType = ._2D;
    image_create_info.format = .D32_SFLOAT_S8_UINT;
    image_create_info.extent.width = vulkan_objects.swap_chain_width;
    image_create_info.extent.height = vulkan_objects.swap_chain_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = ._1_BIT;
    image_create_info.tiling = .OPTIMAL;
    image_create_info.usage = .DEPTH_STENCIL_ATTACHMENT_BIT | .TRANSFER_DST_BIT;
    image_create_info.sharingMode = .EXCLUSIVE;
    image_create_info.initialLayout = .UNDEFINED;

    result = vkCreateImage(vulkan_objects.device, *image_create_info, null, *vulkan_objects.depth_stencil_image);
    if result != .SUCCESS {
        print("vkCreateImage failed for depth stencil buffer: %\n", result);
        return false;
    }

    memory_requirements : VkMemoryRequirements;
    vkGetImageMemoryRequirements(vulkan_objects.device, vulkan_objects.depth_stencil_image, *memory_requirements);

    memory_allocate_info : VkMemoryAllocateInfo;
    memory_allocate_info.allocationSize = memory_requirements.size;
    success, memoryTypeIndex := vulkan_find_memory_by_flag_and_type(vulkan_objects, .DEVICE_LOCAL_BIT,
        memory_requirements.memoryTypeBits);
    if !success {
        print("failed to find memory for depth stencil\n");
        return false;
    }

    result = vkAllocateMemory(vulkan_objects.device, *memory_allocate_info, null,
        *vulkan_objects.depth_stencil_image_memory);
    if result != .SUCCESS {
        print("vkAllocateMemory failed for depth stencil\n");
        return false;
    }

    result = vkBindImageMemory(vulkan_objects.device, vulkan_objects.depth_stencil_image,
        vulkan_objects.depth_stencil_image_memory, 0);
    if result != .SUCCESS {
        print("vkBindImageMemory failed for depth stencil\n");
        return false;
    }

    image_view_create_info : VkImageViewCreateInfo;
    image_view_create_info.image = vulkan_objects.depth_stencil_image;
    image_view_create_info.viewType = ._2D;
    image_view_create_info.format = image_create_info.format;
    image_view_create_info.components.r = .IDENTITY;
    image_view_create_info.components.g = .IDENTITY;
    image_view_create_info.components.b = .IDENTITY;
    image_view_create_info.components.a = .IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = .DEPTH_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(vulkan_objects.device, *image_view_create_info, null,
        *vulkan_objects.depth_stencil_image_view);
    if result != .SUCCESS {
        print("vkCreateImageView failed for depth stencil\n");
        return false;
    }

    return true;
}

init_vulkan_render_pass :: (vulkan_objects: *VulkanObjects) -> bool {
    attachment_descriptions : [2] VkAttachmentDescription;

    {
        using attachment_descriptions[0];

        format = vulkan_objects.swap_chain_format;
        samples = ._1_BIT;
        loadOp = .CLEAR;
        storeOp = .STORE;
        stencilLoadOp = .DONT_CARE;
        stencilStoreOp = .DONT_CARE;
        initialLayout = .UNDEFINED;
        finalLayout = .PRESENT_SRC_KHR;
    }

    {
        using attachment_descriptions[1];

        format = .D32_SFLOAT_S8_UINT;
        samples = ._1_BIT;
        loadOp = .CLEAR;
        storeOp = .STORE;
        stencilLoadOp = .DONT_CARE;
        stencilStoreOp = .STORE;
        initialLayout = .UNDEFINED;
        finalLayout = .DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    colour_attachment_reference : VkAttachmentReference;
    colour_attachment_reference.attachment = 0;
    colour_attachment_reference.layout = .COLOR_ATTACHMENT_OPTIMAL;

    depth_attachment_reference : VkAttachmentReference;
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = .DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass_description : VkSubpassDescription;
    subpass_description.pipelineBindPoint = .GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = *colour_attachment_reference;
    subpass_description.pDepthStencilAttachment = *depth_attachment_reference;

    subpass_dependency : VkSubpassDependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.srcStageMask = .VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = .VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = .VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | .VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass_create_info : VkRenderPassCreateInfo;
    render_pass_create_info.attachmentCount = attachment_descriptions.count;
    render_pass_create_info.pAttachments = attachment_descriptions.data;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = *subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = *subpass_dependency;

    result := vkCreateRenderPass(vulkan_objects.device, *render_pass_create_info, null, *vulkan_objects.render_pass);
    if result != .SUCCESS {
        print("vkCreateRenderPass failed\n");
        return false;
    }

    return true;
}

init_vulkan_frame_buffers :: (vulkan_objects: *VulkanObjects) -> bool {
    attachments : [2] VkImageView;
    attachments[1] = vulkan_objects.depth_stencil_image_view;

    framebuffer_create_info : VkFramebufferCreateInfo;
    framebuffer_create_info.renderPass = vulkan_objects.render_pass;
    framebuffer_create_info.attachmentCount = attachments.count;
    framebuffer_create_info.pAttachments = attachments.data;
    framebuffer_create_info.width = vulkan_objects.swap_chain_width;
    framebuffer_create_info.height = vulkan_objects.swap_chain_height;
    framebuffer_create_info.layers = 1;

    vulkan_objects.framebuffers = NewArray(vulkan_objects.swap_chain_image_count, VkFramebuffer);
    for it,i : vulkan_objects.framebuffers {
        attachments[0] = vulkan_objects.swap_chain_image_views[i];

        result := vkCreateFramebuffer(vulkan_objects.device, *framebuffer_create_info, null,
            *vulkan_objects.framebuffers[i]);
        if result != .SUCCESS {
            print("vkCreateFramebuffer failed\n");
            return false;
        }
    }

    return true;
}

init_vulkan_frame_resource :: (vulkan_objects : VulkanObjects) -> bool, VulkanFrameResource {
    result : VkResult = .ERROR_INITIALIZATION_FAILED;
    frame_resource : VulkanFrameResource;

    fence_create_info : VkFenceCreateInfo;
    fence_create_info.flags = .SIGNALED_BIT;
    result = vkCreateFence(vulkan_objects.device, *fence_create_info, null, *frame_resource.submit_fence);
    if result != .SUCCESS {
        print("vkCreateFence FrameResource failed\n");
        return false, frame_resource;
    }

    command_pool_create_info : VkCommandPoolCreateInfo;
    command_pool_create_info.queueFamilyIndex = vulkan_objects.graphics_queue_index;
    result = vkCreateCommandPool(vulkan_objects.device, *command_pool_create_info, null, *frame_resource.command_pool);
    if result != .SUCCESS {
        print("vkCreateCommandPool FrameResource failed\n");
        return false, frame_resource;
    }

    command_buffer_allocate_info : VkCommandBufferAllocateInfo;
    command_buffer_allocate_info.commandPool = frame_resource.command_pool;
    command_buffer_allocate_info.level = .VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(vulkan_objects.device, *command_buffer_allocate_info,
        *frame_resource.command_buffer);
    if result != .SUCCESS {
        print("vkAllocateCommandBuffers FrameResource failed\n");
        return false, frame_resource;
    }

    return true, frame_resource;
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

    deinit_vulkan_framebuffers(vulkan_objects);

    if vulkan_objects.render_pass
        vkDestroyRenderPass(vulkan_objects.device, vulkan_objects.render_pass, null);

    deinit_vulkan_depth_stencil(vulkan_objects);

    deinit_vulkan_swap_chain_images(vulkan_objects);

    if vulkan_objects.swap_chain
        vkDestroySwapchainKHR(vulkan_objects.device, vulkan_objects.swap_chain, null);

    if vulkan_objects.device
        vkDestroyDevice(vulkan_objects.device, null);

    if vulkan_objects.surface
        vkDestroySurfaceKHR(vulkan_objects.instance, vulkan_objects.surface, null);

    if vulkan_objects.instance
        vkDestroyInstance(vulkan_objects.instance, null);
}

deinit_vulkan_swap_chain_images :: (vulkan_objects : VulkanObjects) {
    for vulkan_objects.swap_chain_image_views {
        if it
            vkDestroyImageView(vulkan_objects.device, it, null);
    }
    free(vulkan_objects.swap_chain_image_views.data);

    for vulkan_objects.swap_chain_resources {
        if it.acquire_image
            vkDestroySemaphore(vulkan_objects.device, it.acquire_image, null);

        if it.release_image
            vkDestroySemaphore(vulkan_objects.device, it.release_image, null);
    }
}

deinit_vulkan_depth_stencil :: (vulkan_objects : VulkanObjects) {
    if vulkan_objects.depth_stencil_image_view
        vkDestroyImageView(vulkan_objects.device, vulkan_objects.depth_stencil_image_view, null);

    if vulkan_objects.depth_stencil_image
        vkDestroyImage(vulkan_objects.device, vulkan_objects.depth_stencil_image, null);

    if vulkan_objects.depth_stencil_image_memory
        vkFreeMemory(vulkan_objects.device, vulkan_objects.depth_stencil_image_memory, null);
}

deinit_vulkan_framebuffers :: (vulkan_objects : VulkanObjects) {
    if vulkan_objects.framebuffers.count > 0 {
        for it, i : vulkan_objects.framebuffers {
            if vulkan_objects.framebuffers[i]
                vkDestroyFramebuffer(vulkan_objects.device, vulkan_objects.framebuffers[i], null);
        }

        free(vulkan_objects.framebuffers.data);
    }
}

deinit_vulkan_frame_resource :: (vulkan_objects : VulkanObjects, frame_resource : VulkanFrameResource) {
    if frame_resource.command_buffer
        vkFreeCommandBuffers(vulkan_objects.device, frame_resource.command_pool, 1, *frame_resource.command_buffer);

    if frame_resource.command_pool
        vkDestroyCommandPool(vulkan_objects.device, frame_resource.command_pool, null);

    if frame_resource.submit_fence
        vkDestroyFence(vulkan_objects.device, frame_resource.submit_fence, null);
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
