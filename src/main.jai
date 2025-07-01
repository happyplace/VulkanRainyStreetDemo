#import "Basic";

#import "Basic";
SDL :: #import "jai-sdl3";

window : *void;

main :: () {
    width : s32 = 1280;
    height : s32 = 720;

    if !SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) {
        print("failed to init SDL: %\n", to_string(SDL_GetError()));
        return;
    }
    defer SDL_Quit();

    window = SDL_CreateWindow("Vulkan Rainy Street Demo", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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

    frame_resource : VulkanFrameResource;
    defer deinit_vulkan_frame_resource(vulkan_objects, frame_resource);
    success, frame_resource = init_vulkan_frame_resource(vulkan_objects);
    if !success
        return;

    quit := false;
    while !quit {
        event : SDL.SDL_Event;
        while SDL_PollEvent(*event) {
            if event.type == {
                case xx SDL_EventType.QUIT;
                    quit = true;
                case xx SDL_EventType.KEY_UP;
                    if event.key.scancode == SDL_Scancode.ESCAPE quit = true;
            }
        }

        u64_min, u64_max := get_integer_range(u64);

        result := vkWaitForFences(vulkan_objects.device, 1, *frame_resource.submit_fence, VK_TRUE, u64_max);
        if result != .SUCCESS
            print("WARN: vkWaitForFences result: %\n", result);

        result = vkAcquireNextImageKHR(vulkan_objects.device, vulkan_objects.swap_chain, u64_max,
            vulkan_objects.swap_chain_resources[vulkan_objects.swap_chain_resource_next_index].acquire_image,
            VK_NULL_HANDLE, *frame_resource.swap_chain_image_index);
        if result == .VK_ERROR_OUT_OF_DATE_KHR {
            // flag window resize
            // abort this frame render
        }
        else if result == .SUBOPTIMAL_KHR {
            // flag window resize
        }
        else if result != .SUCCESS && result != .TIMEOUT && result != .NOT_READY {
            print("ERROR: vkAcquireNextImageKHR failed with result: %\n", result);
            return;
        }

        defer {
            vulkan_objects.swap_chain_resource_next_index += 1;
            if vulkan_objects.swap_chain_resource_next_index >= vulkan_objects.swap_chain_image_count
                vulkan_objects.swap_chain_resource_next_index = 0;
        }

        result = vkResetFences(vulkan_objects.device, 1, *frame_resource.submit_fence);
        if result != .SUCCESS {
            print("ERROR: vkResetFences result: %\n", result);
            return;
        }

        result = vkResetCommandPool(vulkan_objects.device, frame_resource.command_pool, 0);
        if result != .SUCCESS {
            print("ERROR: vkResetFences result: %\n", result);
            return;
        }

        command_buffer_begin_info : VkCommandBufferBeginInfo;
        command_buffer_begin_info.flags = .VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        result = vkBeginCommandBuffer(frame_resource.command_buffer, *command_buffer_begin_info);
        if result != .SUCCESS {
            print("ERROR: vkBeginCommandBuffer result: %\n", result);
            return;
        }

        clear_values : [2] VkClearValue;
        clear_values[0].color._float32 = Vector4.{ 0.39215687, 0.5843138, 0.92941177, 1. }.component;
        clear_values[1].depthStencil.depth = 1;
        clear_values[1].depthStencil.stencil = 0;

        render_pass_begin_info : VkRenderPassBeginInfo;
        render_pass_begin_info.renderPass = vulkan_objects.render_pass;
        render_pass_begin_info.framebuffer = vulkan_objects.framebuffers[frame_resource.swap_chain_image_index];
        render_pass_begin_info.renderArea.extent.width = vulkan_objects.swap_chain_width;
        render_pass_begin_info.renderArea.extent.height = vulkan_objects.swap_chain_height;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.clearValueCount = clear_values.count;
        render_pass_begin_info.pClearValues = clear_values.data;

        vkCmdBeginRenderPass(frame_resource.command_buffer, *render_pass_begin_info, .VK_SUBPASS_CONTENTS_INLINE);

        vkCmdEndRenderPass(frame_resource.command_buffer);

        result = vkEndCommandBuffer(frame_resource.command_buffer);
        if result != .SUCCESS {
            print("ERROR: vkEndCommandBuffer result: %\n", result);
            return;
        }

        submit_info : VkSubmitInfo;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores =
            *vulkan_objects.swap_chain_resources[vulkan_objects.swap_chain_resource_next_index].acquire_image;
        wait_dst_stage_mask := VkPipelineStageFlagBits.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.pWaitDstStageMask = *wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = *frame_resource.command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores =
            *vulkan_objects.swap_chain_resources[vulkan_objects.swap_chain_resource_next_index].release_image;

        present_info : VkPresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores =
            *vulkan_objects.swap_chain_resources[vulkan_objects.swap_chain_resource_next_index].release_image;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = *vulkan_objects.swap_chain;
        present_info.pImageIndices = *frame_resource.swap_chain_image_index;

        result = vkQueueSubmit(vulkan_objects.graphics_queue, 1, *submit_info, frame_resource.submit_fence);
        if result != .SUCCESS {
            print("ERROR: vkEndCommandBuffer result: %\n", result);
            return;
        }

        result = vkQueuePresentKHR(vulkan_objects.graphics_queue, *present_info);
        if result == .VK_ERROR_OUT_OF_DATE_KHR {
            // flag window resize
            // abort this frame render
        }
        else if result == .SUBOPTIMAL_KHR {
            // flag window resize
        }
        else if result != .SUCCESS {
            print("ERROR: failed with result: %\n", result);
            return;
        }
    }

    if vulkan_objects.device
        vkDeviceWaitIdle(vulkan_objects.device);
}
