#ifndef VRSD_GameFrameRender_h_
#define VRSD_GameFrameRender_h_

struct Game;
struct FrameResource;

// gets the next frame resource and will wait if the frame resources are still being rendered on the GPU
FrameResource* game_frame_render_begin_frame(Game* game);

// submit frame resource to GPU
void game_frame_render_end_frame(FrameResource* frame_resource, Game* game);
void game_frame_render_submit(FrameResource* frame_resource, Game* game);

#endif // VRSD_GameFrameRender_h_
