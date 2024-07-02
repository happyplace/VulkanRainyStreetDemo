#ifndef VRSD_ImGuiRenderer_h_
#define VRSD_ImGuiRenderer_h_

struct ImGuiRenderer;

typedef union SDL_Event SDL_Event;

ImGuiRenderer* imgui_renderer_init(struct Game* game);
void imgui_renderer_destroy(struct Game* game, ImGuiRenderer* imgui_renderer);

bool imgui_renderer_on_sdl_event(SDL_Event* sdl_event);
void imgui_renderer_on_resize(struct VulkanRenderer* vulkan_renderer, ImGuiRenderer* imgui_renderer);

void imgui_renderer_draw(struct Game* game, struct FrameResource* frame_resource, ImGuiRenderer* imgui_renderer);

#endif // VRSD_ImGuiRenderer_h_
