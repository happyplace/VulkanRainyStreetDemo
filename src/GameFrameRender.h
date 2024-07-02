#ifndef VRSD_GameFrameRender_h_
#define VRSD_GameFrameRender_h_

struct Game;
struct FrameResource;

// gets the next frame resource and will wait if the frame resources are still being rendered on the GPU
FrameResource* game_frame_render_begin_frame(Game* game);

// submit frame resource to GPU
void game_frame_render_end_frame(Game* game, FrameResource* frame_resource);
void game_frame_render_submit(Game* game, FrameResource* frame_resource);

#endif // VRSD_GameFrameRender_h_
