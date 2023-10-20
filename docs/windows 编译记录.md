下载 v8 库：  https://www.nuget.org/packages/v8-v143-x64/ 

下载 build-tools 
- 安装 clang
- 安装 cmake
- 安装 windows11 sdk


修改 v8相关的 cmakelist.txt

安装下面的依赖
- tree-sitter:     https://www.nuget.org/packages/libtree-sitter-cpp.runtime.win-x64/
- yaml-cpp:        https://www.nuget.org/packages/fluid.yaml-cpp
- LibUV:           https://www.nuget.org/packages/aerospike-client-c-libuv
- abseil:     使用 dependencies/abseil-cpp  进行手动编译



修改编译错误：  
[build] FAILED: imgui_node_editor.dll imgui_node_editor.lib 
*省略了编译命令*
[build] lld-link: error: undefined symbol: struct ImGuiContext *GImGui
[build] >>> referenced by G:\sight\dependencies\imgui\imgui_internal.h:3123
[build] >>>               CMakeFiles/imgui_node_editor.dir/dependencies/imgui-node-editor/imgui_canvas.cpp.obj:(struct ImGuiWindow * __cdecl ImGui::GetCurrentWindow(void))
[build] >>> referenced by G:\sight\dependencies\imgui\imgui_internal.h:3218
[build] >>>               CMakeFiles/imgui_node_editor.dir/dependencies/imgui-node-editor/imgui_node_editor.cpp.obj:(unsigned int __cdecl ImGui::GetActiveID(void))

尝试方案：
- 将动态库修改成静态库   
  - 是可用的。  现在 windows 下会编译静态库， 而 linux 下会编译动态库


参考阅读：

- https://stackoverflow.com/a/12229732/11226492
- https://stackoverflow.com/q/59413012/11226492
  - in comment 



现在，  tree-sitter 没有找到lib 文件， 只有一个dll 。。 
参考这个链接手动编译 tree-sitter:  https://github.com/tree-sitter/tree-sitter/issues/254#issuecomment-1316954986

```cmake
cmake_minimum_required(VERSION 3.23)
project(tree-sitter)

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(tree-sitter STATIC src/lib.c)
target_include_directories(tree-sitter PUBLIC src include)
```


现在 yaml 库的链接出现了问题。。。 
yaml 库来自：   https://globalcdn.nuget.org/packages/fluid.yaml-cpp.1.0.10.nupkg

解决方法：  在yaml.h 文件前面添加一行  `#define YAML_CPP_API`    
原因：  因为下载的是 静态库。。   在`dll.h` 里面会认为当前是一个dll 动态库。。 


现在到v8 了
v8 库来自：  https://globalcdn.nuget.org/packages/v8-v143-x64.11.8.172.15.nupkg

v8有问题的地方在于，  有一个API 被废弃了， 但是头文件里面还有， 只是没有实现了。。。 



