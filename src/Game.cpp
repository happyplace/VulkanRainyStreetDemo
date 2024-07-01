#include "Game.h"

#include "GameWindow.h"
#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"

#include <array>
#include <vulkan/vulkan_core.h>
#include "SDL_assert.h"

struct Game
{
    GameWindow* game_window = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanFrameResources* frame_resources = nullptr;
};

void game_destroy(Game* game)
{
    SDL_assert(game);

    if (game->frame_resources)
    {
        vulkan_frame_resources_destroy(game->vulkan_renderer, game->frame_resources);
    }

    if (game->vulkan_renderer)
    {
       vulkan_renderer_destroy(game->vulkan_renderer);
    }

    if (game->game_window)
    {
       game_window_destory(game->game_window);
    }

    delete game;
}

Game* game_init()
{
    Game* game = new Game();

    game->game_window = game_window_init();
    if (game->game_window == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    game->vulkan_renderer = vulkan_renderer_init(game->game_window);
    if (game->vulkan_renderer == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    game->frame_resources = vulkan_frame_resources_init(game->vulkan_renderer);
    if (game->frame_resources == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    return game;
}

int game_run(int argc, char** argv)
{
    Game* game = game_init();
    if (game == nullptr)
    {
        return 1;
    }

    while (!game->game_window->quit_requested)
    {


        if (game->game_window->window_resized)
        {
            game->game_window->window_resized = false;
            vulkan_renderer_on_window_resized(game->game_window, game->vulkan_renderer);
        }

        game_window_process_events(game->game_window);
    }

    game_destroy(game);
    return 0;
}



    // {
    //     FrameResource* frame_resource = vulkan_frame_resources_get_next_frame_resource(frame_resources);

    //     VkResult result = vkAcquireNextImageKHR(renderer->device, renderer->swapchain, UINT64_MAX, frame_resource->acquire_image, VK_NULL_HANDLE, &frame_resource->swapchain_image_index);
    //     if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
    //     {
    //         // if it's VK_SUBOPTIMAL_KHR or VK_ERROR_OUT_OF_DATE_KHR we'll rebuild the swap chain after the vkQueuePresentKHR
    //         // else every other error result besides VK_SUCCESS is considered an unhandled error
    //         SDL_assert(false);
    //     }

    //     result = vkResetCommandPool(renderer->device, frame_resource->command_pool, 0);
    //     SDL_assert(result == VK_SUCCESS);

    //     VkCommandBufferBeginInfo command_buffer_begin_info;
    //     command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //     command_buffer_begin_info.pNext = nullptr;
    //     command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //     command_buffer_begin_info.pInheritanceInfo = nullptr;
    //     result = vkBeginCommandBuffer(frame_resource->command_buffer, &command_buffer_begin_info);
    //     SDL_assert(result == VK_SUCCESS);

    //     std::array<VkClearValue, 2> clear_values;

    //     clear_values[0].color = {{ 0.392156869f, 0.58431375f, 0.929411769f, 1.0f }};

    //     clear_values[1].depthStencil.depth = 0.0f;
    //     clear_values[1].depthStencil.stencil = 0;

    //     VkRenderPassBeginInfo render_pass_begin_info;
    //     render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //     render_pass_begin_info.pNext = nullptr;
    //     render_pass_begin_info.renderPass = renderer->render_pass;
    //     render_pass_begin_info.framebuffer = renderer->framebuffers[frame_resource->swapchain_image_index];
    //     render_pass_begin_info.renderArea.extent.width = renderer->swapchain_width;
    //     render_pass_begin_info.renderArea.extent.height = renderer->swapchain_height;
    //     render_pass_begin_info.renderArea.offset.x = 0;
    //     render_pass_begin_info.renderArea.offset.y = 0;
    //     render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    //     render_pass_begin_info.pClearValues = clear_values.data();

    //     vkCmdBeginRenderPass(frame_resource->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    //     vkCmdEndRenderPass(frame_resource->command_buffer);

    //     result = vkEndCommandBuffer(frame_resource->command_buffer);
    //     SDL_assert(result == VK_SUCCESS);

    //     VkSubmitInfo submit_info;
    //     submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //     submit_info.pNext = nullptr;
    //     submit_info.waitSemaphoreCount = 1;
    //     submit_info.pWaitSemaphores = &frame_resource->acquire_image;
    //     const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //     submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    //     submit_info.commandBufferCount = 1;
    //     submit_info.pCommandBuffers = &frame_resource->command_buffer;
    //     submit_info.signalSemaphoreCount = 1;
    //     submit_info.pSignalSemaphores = &frame_resource->release_image;
    //     result = vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, frame_resource->submit_fence);
    //     SDL_assert(result == VK_SUCCESS);

    //     VkPresentInfoKHR present_info;
    //     present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //     present_info.pNext = nullptr;
    //     present_info.waitSemaphoreCount = 1;
    //     present_info.pWaitSemaphores = &frame_resource->release_image;
    //     present_info.swapchainCount = 1;
    //     present_info.pSwapchains = &renderer->swapchain;
    //     present_info.pImageIndices = &frame_resource->swapchain_image_index;
    //     present_info.pResults = nullptr;

    //     result = vkQueuePresentKHR(renderer->graphics_queue, &present_info);
    //     if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    //     {
    //         window->window_resized = false;
    //         vulkan_renderer_on_window_resized(window, renderer);
    //     }
    //     else if (result != VK_SUCCESS)
    //     {
    //         SDL_assert(result == VK_SUCCESS);
    //     }

    //     result = vkWaitForFences(renderer->device, 1, &frame_resource->submit_fence, VK_TRUE, UINT64_MAX);
    //     SDL_assert(result == VK_SUCCESS);
    //     result = vkResetFences(renderer->device, 1, &frame_resource->submit_fence);
    //     SDL_assert(result == VK_SUCCESS);


    // }
