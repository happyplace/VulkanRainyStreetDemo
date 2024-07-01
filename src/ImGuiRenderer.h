#ifndef VRSD_ImGuiRenderer_h_
#define VRSD_ImGuiRenderer_h_

struct ImGuiRenderer;

ImGuiRenderer* imgui_renderer_init(struct Game* game);
void imgui_renderer_destroy(struct Game* game, ImGuiRenderer* game_map_renderer);

#endif // VRSD_ImGuiRenderer_h_
