#include "ImGuiRenderer.h"
#include <array>
#include <vulkan/vulkan_core.h>

#ifdef IMGUI_ENABLED

#include <vulkan/vulkan.h>

#include "SDL_assert.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "Game.h"
#include "GameWindow.h"
#include "VulkanRenderer.h"

struct ImGuiRenderer
{
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
};

void imgui_renderer_init_assert(VkResult result)
{
    VK_ASSERT(result);
}

void imgui_renderer_destroy(struct Game* game, ImGuiRenderer* imgui_renderer)
{
    SDL_assert(imgui_renderer);

    vkDeviceWaitIdle(game->vulkan_renderer->device);

    if (imgui_renderer->descriptor_pool)
    {
        vkDestroyDescriptorPool(game->vulkan_renderer->device, imgui_renderer->descriptor_pool, s_allocator);
    }

    if (imgui_renderer->render_pass)
    {
        vkDestroyRenderPass(game->vulkan_renderer->device, imgui_renderer->render_pass, s_allocator);
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    delete imgui_renderer;
}

ImGuiRenderer* imgui_renderer_init(struct Game* game)
{
    ImGuiRenderer* imgui_renderer = new ImGuiRenderer();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    if (!ImGui_ImplSDL2_InitForVulkan(game->game_window->window))
    {
        imgui_renderer_destroy(game, imgui_renderer);
        return nullptr;
    }

    std::array<VkDescriptorPoolSize, 1> pool_sizes;
    pool_sizes[0] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info;
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();

    VkResult result = vkCreateDescriptorPool(game->vulkan_renderer->device, &descriptor_pool_create_info, s_allocator, &imgui_renderer->descriptor_pool);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);

        imgui_renderer_destroy(game, imgui_renderer);
        return nullptr;
    }

    VkAttachmentDescription attachment_description;
    attachment_description.flags = 0;
    attachment_description.format = game->vulkan_renderer->swapchain_format;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_attachment_reference;
    colour_attachment_reference.attachment = 0;
    colour_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description;
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &colour_attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    VkSubpassDependency subpass_dependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    result = vkCreateRenderPass(game->vulkan_renderer->device, &render_pass_create_info, s_allocator, &imgui_renderer->render_pass);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);

        imgui_renderer_destroy(game, imgui_renderer);
        return nullptr;
    }

    ImGui_ImplVulkan_InitInfo imgui_impl_vulkan_init_info;

    memset(&imgui_impl_vulkan_init_info, 0, sizeof(imgui_impl_vulkan_init_info));

    imgui_impl_vulkan_init_info.Instance = game->vulkan_renderer->instance;
    imgui_impl_vulkan_init_info.PhysicalDevice = game->vulkan_renderer->physical_device;
    imgui_impl_vulkan_init_info.Device = game->vulkan_renderer->device;
    imgui_impl_vulkan_init_info.QueueFamily = game->vulkan_renderer->graphics_queue_index;
    imgui_impl_vulkan_init_info.Queue = game->vulkan_renderer->graphics_queue;
    imgui_impl_vulkan_init_info.DescriptorPool = imgui_renderer->descriptor_pool;
    imgui_impl_vulkan_init_info.RenderPass = imgui_renderer->render_pass;
    imgui_impl_vulkan_init_info.MinImageCount = game->vulkan_renderer->swapchain_image_count;
    imgui_impl_vulkan_init_info.ImageCount = game->vulkan_renderer->swapchain_image_count;
    imgui_impl_vulkan_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    //imgui_impl_vulkan_init_info.PipelineCache = VK_NULL_HANDLE;
    //imgui_impl_vulkan_init_info.Subpass = 0;

    //imgui_impl_vulkan_init_info.UseDynamicRendering = false;
//#ifdef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
    //imgui_impl_vulkan_init_info.PipelineRenderingCreateInfo;
//#endif

    // imgui_impl_vulkan_init_info.Allocator = s_allocator;
    // imgui_impl_vulkan_init_info.CheckVkResultFn = imgui_renderer_init_assert;
    // imgui_impl_vulkan_init_info.MinAllocationSize = 0;

    if (!ImGui_ImplVulkan_Init(&imgui_impl_vulkan_init_info))
    {
        SDL_assert(false);

        imgui_renderer_destroy(game, imgui_renderer);
        return nullptr;
    }

    ImGui_ImplVulkan_SetMinImageCount(game->vulkan_renderer->swapchain_image_count);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    return imgui_renderer;
}

#endif // IMGUI_ENABLED