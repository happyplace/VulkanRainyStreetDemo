# Rainy Street Demo

## Feature Updates:
- [x] Multiple frames in flight
- [x] ~~Fiber tasking library~~

## New Features:
 - [ ] Instancing
 - [ ] Shadow Mapping
 - [ ] Ambient Occlusion
 - [ ] Character Animation

## Scene Elements:
 - [ ] Woman running an idle animation in front of the camera
 - [ ] Characters waiting for the light to change and then walking across the street
 - [ ] Cars driving and turning
 - [ ] Working street lights with cars and characters following street lights
 - [ ] Rain shader
 - [ ] Cars and characters should loop and be recycled so the scene runs forever

![benchmark image for what I want the final demo to look like, it has a lady standing in front of the camera on a street corner with people crossing the street and cars in the intersection](rainy_street_goal.jpg)

> target for what the final scene should look like

## External Libraries:
| Name | Path |
| ------ | ------ |
| DirectXMath | external/DirectXMath |
| .NET Runtime (**sal.h**) | external/dotnet_runtime |
| Dear ImGui | external/imgui-docking |
| Vulkan Memory Allocator | external/VulkanMemoryAllocator |
| Shaderc | external/shaderc |
| KhronosGroup glslang | external/shaderc/third_party/glslang |
| SPIR-V Tools | external/shaderc/third_party/spirv-tools |
| SPIR-V Headers | external/shaderc/third_party/spirv-tools/external/spirv-headers |
| SDL 2 | *(links against system library)* |
| Vulkan | *(links against system library)* |
