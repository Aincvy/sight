
#include "sight_render.h"

#include "sight_widgets.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>


// todo change this
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


namespace sight {
    

    int initWindowBackend() {

        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return CODE_FAIL;

        // GL 3.2 + GLSL 150
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);     // 3.2+ only

#    ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);     // Required on Mac
#    endif

        return CODE_OK;
    }

    void* initWindow(const char* title, int width, int height, std::function<void(ImGuiIO&)> initImgui) {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, NULL);
        if (window == nullptr)
            return nullptr;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);     // Enable vsync

        // Setup Platform/Renderer backends
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        initImgui(io);

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char* glsl_version = "#version 150";
        ImGui_ImplOpenGL3_Init(glsl_version);

        return window;
    }

    void mainLoopWindow(void* glWindow, uv_loop_t* uvLoop, bool& exitFlag, std::function<int()> beforeRenderFunc, std::function<void()> render_func) {

        // Main loop

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        GLFWwindow* window = (GLFWwindow*)glWindow;

        while (!glfwWindowShouldClose(window) && !exitFlag) {
            if (beforeRenderFunc() != CODE_OK) {
                break;
            }

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

            render_func();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }

    void cleanUpWindow(void* glWindow) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow((GLFWwindow*)glWindow);
    }

    void terminateBackend() {
        glfwTerminate();
    }

}