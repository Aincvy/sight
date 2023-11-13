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
上面的版本有问题， 要使用这个版本：   https://globalcdn.nuget.org/packages/v8-v143-x64.11.9.169.4.nupkg   

v8有问题的地方在于，  有一个API 被废弃了， 但是头文件里面还有， 只是没有实现了。。。 

从这里下载 v8库的dll文件：  https://globalcdn.nuget.org/packages/v8.redist-v143-x64.11.9.169.4.nupkg
解压后放到build 目录里面

使用这个工具可以查看符号和dll依赖：  https://github.com/lucasg/Dependencies



启动的时候， 有问题， 应该是dll 的加载有问题。。   
现在考虑的方向
- 使用 msvc 编译项目
  - no luck       和clang 一样的错误。。。 
  - 屏蔽一下 sight_js.cpp 文件， 看看能不能启动。 
  - yaml-test 里面，  即使加入了v8 也还是能成功启动到main 函数
  - 屏蔽了 sight_js.cpp 之后， 确实能启动了。。  但是目前的问题是，这个文件非常巨大。。  我日。。
  - 应该还是和v8pp 有关系， 目前bindXX 函数是有问题的。。。 
  - `v8::Exception::TypeError(v8pp::to_v8(isolate, "Not impl."))`   TypeError 居然是一个v8里面的类型！ 我草了， 我以为是windows 给的提示呢。。 
  - 原因是 lib文件和 dll 文件里面的符号表不一致。。  日了。  ~~所以我现在要自己手动的编译v8 ~~
  - 确实不一样， 是我下载 nuget 的版本不一样。。  早知道还是使用nuget 下载了， 而不是手动下载。。。 
- 使用clang 本地编译v8 
  - 先尝试用 clang把， 不行的话， 再使用msvc 

使用一个简单的项目进行了一些测试， 我发现确实是这样的， 使用clang编译的话， 就无法使用这个dll , 如果使用 msvc的话就可以。。 

所以现在可能需要修改代码， 以实现 msvc的编译。。 


复制 others/v8pp/config.hpp 到 dep.../v8pp/v8pp/  目录里面
