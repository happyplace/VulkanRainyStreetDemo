#ifndef VRSD_ImGuiRenderer_h_
#define VRSD_ImGuiRenderer_h_

struct ImGuiRenderer;

typedef union SDL_Event SDL_Event;

ImGuiRenderer* imgui_renderer_init(struct Game* game);
void imgui_renderer_destroy(ImGuiRenderer* imgui_renderer, struct Game* game);

bool imgui_renderer_on_sdl_event(SDL_Event* sdl_event);
void imgui_renderer_on_resize(ImGuiRenderer* imgui_renderer, struct VulkanRenderer* vulkan_renderer);

void imgui_renderer_draw(ImGuiRenderer* imgui_renderer, struct Game* game, struct FrameResource* frame_resource);

#endif // VRSD_ImGuiRenderer_h_
