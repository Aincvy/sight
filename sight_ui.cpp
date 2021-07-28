#include "sight_ui.h"
#include "sight_node_editor.h"
#include "sight.h"
#include "sight_js.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

// ignore loader part.
//#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <GL/gl3w.h>

#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


static sight::UIStatus* g_UIStatus = nullptr;

namespace sight {

    namespace {
        // private members and functions.

        void showMainFileMenu(UIStatus& uiStatus){
            if (ImGui::MenuItem("New")){

            }

            if (ImGui::MenuItem("Open", "Ctrl+O")) {

            }

            ImGui::Separator();
            if (ImGui::MenuItem("Options")) {
                // show options window

            }

            if (ImGui::MenuItem("Exit")){
                uiStatus.closeWindow = true;
            }
        }

        void showMainEditMenu(){
            if (ImGui::MenuItem("Undo")){

            }

            if (ImGui::MenuItem("Redo")) {

            }
        }

        void showMainViewMenu(){
            if (ImGui::MenuItem("View")) {

            }
        }

        void showMainBuildMenu(){
            if (ImGui::MenuItem("Build")) {

            }
            if (ImGui::MenuItem("Rebuild")) {

            }
            if (ImGui::MenuItem("Clean")) {

            }
        }

        void showMainCustomMenu(){
            if (ImGui::MenuItem("Trigger")) {
                addJsCommand(JsCommandType::File, "/Volumes/mac_extend/Project/sight/scripts/nodes.js");
            }
        }

        void showMainMenuBar(UIStatus& uiStatus){
            if (ImGui::BeginMainMenuBar()){
                if (ImGui::BeginMenu("File")) {
                    showMainFileMenu(uiStatus);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    showMainEditMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View")) {
                    showMainViewMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Build")) {
                    showMainBuildMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Custom")) {
                    showMainCustomMenu();
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        void showDemoWindow(){
            ImGui::Begin("Test Window");
            ImGui::Text("this is first line.");
            ImGui::End();
        }

        void showHierarchyWindow(bool needInit){
            if (needInit) {
                ImGui::SetNextWindowPos(ImVec2(0,20));
                ImGui::SetNextWindowSize(ImVec2(300, 285));
            }
            ImGui::Begin("Hierarchy");
            // it's should be what ?
            ImGui::End();
        }

        void showInspectorWindow(bool needInit){
            if (needInit) {
                ImGui::SetNextWindowPos(ImVec2(0,305));
                ImGui::SetNextWindowSize(ImVec2(300, 300));
            }

            ImGui::Begin("Inspector");
            // what's here?
            // it's should be show what user selected.
            ImGui::End();
        }

    }

    void mainWindowFrame(UIStatus & uiStatus) {
        showMainMenuBar(uiStatus);

        //showDemoWindow();
        // windows
        showHierarchyWindow(uiStatus.needInit);
        showInspectorWindow(uiStatus.needInit);
        showNodeEditorGraph(uiStatus);

    }

    void runUICommand(UICommand *command){
        printf("runUICommand...");
        switch (command->type) {
            case UICommandType::UICommandHolder:
                break;
            case UICommandType::COMMON:
                break;
            case UICommandType::AddNode:
            {
                // command->args.needFree = false;
                auto* nodePointer = (SightNode**) command->args.data;
                auto size = command->args.dataLength;
                for (int i = 0; i < size; ++i) {
                    addNode(nodePointer[i]);
                }
                break;
            }
        }

        command->args.dispose();
    }

    void runUICommandCallback(uv_async_t *handle){
        auto *command = (UICommand *)handle->data;
        runUICommand(command);
        free(command);
    }

    int showMainWindow(){

        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;

        // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac

        // Create window with graphics context
        GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
        if (window == NULL)
            return 1;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        bool err = gl3wInit() != 0;
        if (err)
        {
            fprintf(stderr, "Failed to initialize OpenGL loader!\n");
            return 1;
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Our state
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        UIStatus uiStatus = {
            true,
            io,
            false,
        };
        g_UIStatus = &uiStatus;
        uv_async_t uvAsync;
        uiStatus.uvAsync = &uvAsync;

        // uv loop init
        auto uvLoop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uiStatus.uvLoop = uvLoop;
        uv_loop_init(uvLoop);
        uv_async_init(uvLoop, uiStatus.uvAsync, runUICommandCallback);

        initNodeEditor();

        // Main loop
        while (!glfwWindowShouldClose(window) && !uiStatus.closeWindow)
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            mainWindowFrame(uiStatus);


            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);

            uiStatus.needInit = false;
            // printf("before uv run\n");
            uv_run(uvLoop, UV_RUN_NOWAIT);
        }

        destroyNodeEditor();

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();

        uv_loop_close(uvLoop);
        free(uvLoop);
        uiStatus.uvLoop = nullptr;
        exitSight();

        return 0;
    }

    void addUICommand(UICommandType type, int argInt) {
        UICommand command = {
                type,
                {
                        .argInt = argInt
                }
        };

        addUICommand(command);
    }


    int addUICommand(UICommandType type, void *data, size_t dataLength, bool needFree) {
        UICommand command = {
                type,
                {
                        .needFree = needFree,
                        .data =  data,
                        .dataLength = dataLength,
                }
        };

        return addUICommand(command);
    }

    int addUICommand(UICommand &command) {
        auto async = g_UIStatus->uvAsync;
        async->data = copyObject(&command, sizeof(UICommand));
        uv_async_send(async);
        printf("addUICommand ...\n");
        return 0;
    }



}
