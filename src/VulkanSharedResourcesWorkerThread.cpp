#include "VulkanSharedResourcesWorkerThread.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>

#include <SDL_assert.h>
#include "vk_mem_alloc.h"

#include "Game.h"
#include "VulkanSharedResources.h"
#include "VulkanRenderer.h"

struct VulkanSharedResourcesWorkerThreadData
{
    std::vector<std::thread> worker_threads;
    std::atomic_bool exit_worker_thread = false;
    std::condition_variable conditional_variable;
    std::mutex conditional_variable_mutex;
    std::queue<VulkanResourceTransferBufferParams*> transfer_queue;
    // TODO: should be a spinning mutex to avoid sleeping worker threads
    std::mutex transfer_queue_lock;
    std::mutex vulkan_queue_mutex;
};

constexpr int c_vulkan_shared_resources_worker_thread_count = 10;

static VulkanSharedResourcesWorkerThreadData* s_worker_thread_data_instance = nullptr;

void THREAD_vulkan_shared_resources_worker_thread(VulkanRenderer* vulkan_renderer)
{
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    {
        VkFenceCreateInfo fence_create_info;
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_ASSERT(vkCreateFence(vulkan_renderer->device, &fence_create_info, nullptr, &fence));

        VkCommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = nullptr;
        command_pool_create_info.flags = 0;
        command_pool_create_info.queueFamilyIndex = vulkan_renderer->transfer_queue_index;
        VK_ASSERT(vkCreateCommandPool(vulkan_renderer->device, &command_pool_create_info, nullptr, &command_pool));

        VkCommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = nullptr;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
        VK_ASSERT(vkAllocateCommandBuffers(vulkan_renderer->device, &command_buffer_allocate_info, &command_buffer));
    }

    while (!s_worker_thread_data_instance->exit_worker_thread.load(std::memory_order_release))
    {
        VulkanResourceTransferBufferParams* transfer_buffer_params = nullptr;
        {
            std::unique_lock queue_lock(s_worker_thread_data_instance->transfer_queue_lock);
            if (!s_worker_thread_data_instance->transfer_queue.empty())
            {
                transfer_buffer_params = s_worker_thread_data_instance->transfer_queue.front();
                s_worker_thread_data_instance->transfer_queue.pop();
            }
        }

        if (transfer_buffer_params == nullptr)
        {
            std::unique_lock scoped_lock(s_worker_thread_data_instance->conditional_variable_mutex);
            s_worker_thread_data_instance->conditional_variable.wait(scoped_lock);
        }
        else
        {
            VkBufferCreateInfo buffer_create_info;
            buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_create_info.pNext = nullptr;
            buffer_create_info.flags = 0;
            buffer_create_info.size = static_cast<VkDeviceSize>(transfer_buffer_params->data_size);
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            buffer_create_info.queueFamilyIndexCount = 0;
            buffer_create_info.pQueueFamilyIndices = nullptr;

            VmaAllocationCreateInfo allocation_create_info;
            allocation_create_info.flags = 
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
            allocation_create_info.requiredFlags = 0;
            allocation_create_info.preferredFlags = 0;
            allocation_create_info.memoryTypeBits = 0;
            allocation_create_info.pool = VK_NULL_HANDLE;
            allocation_create_info.pUserData = nullptr;
            allocation_create_info.priority = 0.0f;

            VkBuffer staging_buffer;
            VmaAllocation staging_allocation;
            VmaAllocationInfo staging_allocation_info;
            VkResult result = vmaCreateBuffer(
                transfer_buffer_params->vulkan_renderer->vma_allocator, 
                &buffer_create_info, 
                &allocation_create_info, 
                &staging_buffer, 
                &staging_allocation, 
                &staging_allocation_info);
            VK_ASSERT(result);
            if (result != VK_SUCCESS)
            {
                delete transfer_buffer_params;
                continue;
            }

            memcpy(staging_allocation_info.pMappedData, 
                   transfer_buffer_params->data, 
                   transfer_buffer_params->data_size);
            VK_ASSERT(vmaFlushAllocation(
                transfer_buffer_params->vulkan_renderer->vma_allocator, 
                staging_allocation, 
                0, 
                VK_WHOLE_SIZE));

            VK_ASSERT(vkWaitForFences(
                transfer_buffer_params->vulkan_renderer->device, 
                1, 
                &fence, 
                VK_TRUE, 
                UINT64_MAX));

            VK_ASSERT(vkResetFences(
                transfer_buffer_params->vulkan_renderer->device, 
                1, 
                &fence));
            VK_ASSERT(vkResetCommandPool(
                transfer_buffer_params->vulkan_renderer->device, 
                command_pool, 
                0));

            VkCommandBufferBeginInfo command_buffer_begin_info;
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.pNext = nullptr;
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            command_buffer_begin_info.pInheritanceInfo = nullptr;
            VK_ASSERT(vkBeginCommandBuffer(
                command_buffer, 
                &command_buffer_begin_info));

            VkBufferCopy buffer_copy;
            buffer_copy.srcOffset = 0;
            buffer_copy.dstOffset = 0;
            buffer_copy.size = static_cast<VkDeviceSize>(transfer_buffer_params->data_size);
            vkCmdCopyBuffer(
                command_buffer, 
                staging_buffer, 
                transfer_buffer_params->buffer, 
                1, 
                &buffer_copy);

            VK_ASSERT(vkEndCommandBuffer(command_buffer));

            VkSubmitInfo submit_info;
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = nullptr;
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = nullptr;
            submit_info.pWaitDstStageMask = nullptr;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = nullptr;

            {
                std::unique_lock queue_lock(s_worker_thread_data_instance->vulkan_queue_mutex);
                VK_ASSERT(vkQueueSubmit(
                    transfer_buffer_params->vulkan_renderer->transfer_queue, 
                    1, 
                    &submit_info, 
                    fence));
            }

            VK_ASSERT(vkWaitForFences(
                transfer_buffer_params->vulkan_renderer->device, 
                1, 
                &fence, 
                VK_TRUE, 
                UINT64_MAX));

            vmaDestroyBuffer(
                transfer_buffer_params->vulkan_renderer->vma_allocator, 
                staging_buffer, 
                staging_allocation);

            transfer_buffer_params->vulkan_mesh_resource->resource_loaded_count.fetch_add(1);

            delete transfer_buffer_params;
        }
    }
    
    vkDestroyFence(vulkan_renderer->device, fence, nullptr);
    vkFreeCommandBuffers(vulkan_renderer->device, command_pool, 1, &command_buffer);
    vkDestroyCommandPool(vulkan_renderer->device, command_pool, nullptr);
}

void vulkan_shared_resources_init_worker_thread_task(ftl::TaskScheduler* /*task_scheduler*/, void* arg)
{
    VulkanRenderer* vulkan_renderer = reinterpret_cast<VulkanRenderer*>(arg);

    s_worker_thread_data_instance = new VulkanSharedResourcesWorkerThreadData();
    s_worker_thread_data_instance->exit_worker_thread.store(false, std::memory_order_release);

    for (int i = 0; i < c_vulkan_shared_resources_worker_thread_count; ++i)
    {
        s_worker_thread_data_instance->worker_threads.emplace_back(
            THREAD_vulkan_shared_resources_worker_thread, vulkan_renderer);
    }
}

void vulkan_shared_resources_destroy_worker_thread()
{
    SDL_assert(s_worker_thread_data_instance);

    s_worker_thread_data_instance->exit_worker_thread.store(true, std::memory_order_release);
    s_worker_thread_data_instance->conditional_variable.notify_all();

    for (std::thread& worker_thread : s_worker_thread_data_instance->worker_threads)
    {
        worker_thread.join();
    }

    delete s_worker_thread_data_instance;
    s_worker_thread_data_instance = nullptr;
}

void vulkan_shared_resources_add_transfer_buffer_params(VulkanResourceTransferBufferParams* transfer_buffer_params)
{
    {
        std::unique_lock scoped_lock(s_worker_thread_data_instance->transfer_queue_lock);
        s_worker_thread_data_instance->transfer_queue.push(transfer_buffer_params);
    }
    s_worker_thread_data_instance->conditional_variable.notify_one();
}

