#import "Basic";
#import "Compiler";
#import "File";
FileUtils :: #import "File_Utilities";

#run build();

w : Workspace;

build :: () {
    make_directory_if_it_does_not_exist("bin");

    w = compiler_create_workspace("Target Program");
    if !w {
        print("Workspace creation failed.\n");
        return;
    }

    target_options := get_build_options(w);
    target_options.output_executable_name = "rainy_street";
    target_options.output_path = "bin";
    set_build_options(target_options, w);

    directory_visitor_func :: (info: *FileUtils.File_Visit_Info, success_pointer: *bool) {
        add_build_file(info.full_name, w);
    }
    success := true;
    FileUtils.visit_files("src", recursive=true, *success, directory_visitor_func, 
        visit_files=true, visit_directories=false);

    set_build_options_dc(.{do_output=false});
}

