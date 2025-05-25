#import "Basic";
#import "Input";
#import "Window_Creation";

main :: () {
    width : u32 = 1280;
    height : u32 = 720;

    window := create_window(width, height, "Vulkan Rainy Street Demo");

    quit := false;
    while !quit {
        update_window_events();

        for events_this_frame {
            if it.type == .QUIT then quit = true;
        }
    }
}

