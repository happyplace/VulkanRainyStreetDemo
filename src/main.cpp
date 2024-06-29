#include "GameWindow.h"
#include "VulkanRenderer.h"

#include <array>
#include <vulkan/vulkan_core.h>
#include "SDL_assert.h"

int main(int argc, char** argv)
{
    GameWindow* window = game_window_init();
    if (window == nullptr)
    {
        return 1;
    }

    VulkanRenderer* renderer = vulkan_renderer_init(window);
    if (renderer == nullptr)
    {
        return 1;
    }

    VkSemaphore acquire_image = VK_NULL_HANDLE;
    VkSemaphore release_image = VK_NULL_HANDLE;
    VkFence submit_fence = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    VkResult result = vkCreateSemaphore(renderer->device, &semaphore_create_info, nullptr, &acquire_image);
    SDL_assert(result == VK_SUCCESS);

    result = vkCreateSemaphore(renderer->device, &semaphore_create_info, nullptr, &release_image);
    SDL_assert(result == VK_SUCCESS);

    VkFenceCreateInfo fence_create_info;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = 0;
    result = vkCreateFence(renderer->device, &fence_create_info, nullptr, &submit_fence);
    SDL_assert(result == VK_SUCCESS);

    VkCommandPoolCreateInfo command_pool_create_info;
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = 0;
    command_pool_create_info.queueFamilyIndex = renderer->graphics_queue_index;
    result = vkCreateCommandPool(renderer->device, &command_pool_create_info, nullptr, &command_pool);
    SDL_assert(result == VK_SUCCESS);

    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(renderer->device, &command_buffer_allocate_info, &command_buffer);
    SDL_assert(result == VK_SUCCESS);

    while (!window->quit_requested)
    {
        uint32_t swapchain_image_index;
        VkResult result = vkAcquireNextImageKHR(renderer->device, renderer->swapchain, UINT64_MAX, acquire_image, VK_NULL_HANDLE, &swapchain_image_index);
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
        {
            // if it's VK_SUBOPTIMAL_KHR or VK_ERROR_OUT_OF_DATE_KHR we'll rebuild the swap chain after the vkQueuePresentKHR
            // else every other error result besides VK_SUCCESS is considered an unhandled error
            SDL_assert(false);
        }

        result = vkResetCommandPool(renderer->device, command_pool, 0);
        SDL_assert(result == VK_SUCCESS);

        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        SDL_assert(result == VK_SUCCESS);

        std::array<VkClearValue, 2> clear_values;

        clear_values[0].color = {{ 0.392156869f, 0.58431375f, 0.929411769f, 1.0f }};

        clear_values[1].depthStencil.depth = 0.0f;
        clear_values[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo render_pass_begin_info;
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = renderer->render_pass;
        render_pass_begin_info.framebuffer = renderer->framebuffers[swapchain_image_index];
        render_pass_begin_info.renderArea.extent.width = renderer->swapchain_width;
        render_pass_begin_info.renderArea.extent.height = renderer->swapchain_height;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdEndRenderPass(command_buffer);

        result = vkEndCommandBuffer(command_buffer);
        SDL_assert(result == VK_SUCCESS);

        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &acquire_image;
        const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &release_image;
        result = vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, submit_fence);
        SDL_assert(result == VK_SUCCESS);

        VkPresentInfoKHR present_info;
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &release_image;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &renderer->swapchain;
        present_info.pImageIndices = &swapchain_image_index;
        present_info.pResults = nullptr;

        result = vkQueuePresentKHR(renderer->graphics_queue, &present_info);
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            window->window_resized = false;
            vulkan_renderer_on_window_resized(window, renderer);
        }
        else if (result != VK_SUCCESS)
        {
            SDL_assert(result == VK_SUCCESS);
        }

        result = vkWaitForFences(renderer->device, 1, &submit_fence, VK_TRUE, UINT64_MAX);
        SDL_assert(result == VK_SUCCESS);
        result = vkResetFences(renderer->device, 1, &submit_fence);
        SDL_assert(result == VK_SUCCESS);

        game_window_process_events(window);

        if (window->window_resized)
        {
            window->window_resized = false;
            vulkan_renderer_on_window_resized(window, renderer);
        }
    }

    vkFreeCommandBuffers(renderer->device, command_pool, 1, &command_buffer);
    vkDestroyCommandPool(renderer->device, command_pool, nullptr);
    vkDestroyFence(renderer->device, submit_fence, nullptr);
    vkDestroySemaphore(renderer->device, acquire_image, nullptr);
    vkDestroySemaphore(renderer->device, release_image, nullptr);

    vulkan_renderer_destroy(renderer);
    game_window_destory(window);
    return 0;
}
