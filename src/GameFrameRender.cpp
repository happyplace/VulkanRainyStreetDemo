#include "GameFrameRender.h"

#include <array>
#include <vulkan/vulkan_core.h>

#include <SDL_assert.h>

#include "Game.h"
#include "GameWindow.h"
#include "VulkanFrameResources.h"
#include "VulkanRenderer.h"

FrameResource* game_frame_render_begin_frame(Game* game)
{
    VkResult result = VK_SUCCESS;

    FrameResource* frame_resource = vulkan_frame_resources_peek_next_frame_resource(game->frame_resources);

    // make sure all the work on this frame resource is done before attempting to use it for another frame
    result = vkWaitForFences(game->vulkan_renderer->device, 1, &frame_resource->submit_fence, VK_TRUE, UINT64_MAX);
    SDL_assert(result == VK_SUCCESS);

    result = vkAcquireNextImageKHR(
        game->vulkan_renderer->device,
        game->vulkan_renderer->swapchain,
        UINT64_MAX,
        frame_resource->acquire_image,
        VK_NULL_HANDLE,
        &frame_resource->swapchain_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::ResizeRequested, true);
        return nullptr;
    }
    else if (result == VK_SUBOPTIMAL_KHR)
    {
        // this counts as an success, we can render the frame but next frame we're going to rebuild the swap chain
        game_window_set_window_flag(game->game_window, GameWindowFlag::ResizeRequested, true);
    }
    else if (result != VK_SUCCESS && result != VK_TIMEOUT && result != VK_NOT_READY)
    {
        // if it's not any of the the success result answers then it's some unhandled issue
        SDL_assert(false);
    }

    vulkan_frame_resources_pop_next_frame_resource(game->frame_resources);

    result = vkResetFences(game->vulkan_renderer->device, 1, &frame_resource->submit_fence);
    SDL_assert(result == VK_SUCCESS);

    result = vkResetCommandPool(game->vulkan_renderer->device, frame_resource->command_pool, 0);
    SDL_assert(result == VK_SUCCESS);

    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    result = vkBeginCommandBuffer(frame_resource->command_buffer, &command_buffer_begin_info);
    SDL_assert(result == VK_SUCCESS);

    std::array<VkClearValue, 2> clear_values;

    clear_values[0].color = {{ 0.392156869f, 0.58431375f, 0.929411769f, 1.0f }};

    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = game->vulkan_renderer->render_pass;
    render_pass_begin_info.framebuffer = game->vulkan_renderer->framebuffers[frame_resource->swapchain_image_index];
    render_pass_begin_info.renderArea.extent.width = game->vulkan_renderer->swapchain_width;
    render_pass_begin_info.renderArea.extent.height = game->vulkan_renderer->swapchain_height;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(frame_resource->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    return frame_resource;
}

void game_frame_render_end_frame(FrameResource* frame_resource, Game* /*game*/)
{
    vkCmdEndRenderPass(frame_resource->command_buffer);
}

void game_frame_render_submit(FrameResource* frame_resource, Game* game)
{
    VkResult result = vkEndCommandBuffer(frame_resource->command_buffer);
    SDL_assert(result == VK_SUCCESS);

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame_resource->acquire_image;
    const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame_resource->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame_resource->release_image;

    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame_resource->release_image;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &game->vulkan_renderer->swapchain;
    present_info.pImageIndices = &frame_resource->swapchain_image_index;
    present_info.pResults = nullptr;

    result = vkQueueSubmit(game->vulkan_renderer->graphics_queue, 1, &submit_info, frame_resource->submit_fence);
    SDL_assert(result == VK_SUCCESS);

    result = vkQueuePresentKHR(game->vulkan_renderer->graphics_queue, &present_info);

    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::ResizeRequested, true);
    }
    else if (result != VK_SUCCESS)
    {
        SDL_assert(result == VK_SUCCESS);
    }
}
