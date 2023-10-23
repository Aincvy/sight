
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

    void initKeys() {
        // ctrl, alt, win/super, shift
        keyMap["lctrl"] = keyArray.add(SightKey(GLFW_KEY_LEFT_CONTROL));
        keyMap["rctrl"] = keyArray.add(SightKey(GLFW_KEY_RIGHT_CONTROL));
        keyMap["ctrl"] = doubleKeyArray.add(SightTwoKey(keyMap["lctrl"], keyMap["rctrl"], false));


        keyMap["lsuper"] = keyArray.add(SightKey(GLFW_KEY_LEFT_SUPER));
        keyMap["rsuper"] = keyArray.add(SightKey(GLFW_KEY_RIGHT_SUPER));
        keyMap["super"] = doubleKeyArray.add(SightTwoKey(keyMap["lsuper"], keyMap["rsuper"], false));
        // some compatibility
        keyMap["lwin"] = keyMap["lsuper"];
        keyMap["rwin"] = keyMap["rsuper"];
        keyMap["win"] = keyMap["super"];
        keyMap["lcmd"] = keyMap["lsuper"];
        keyMap["rcmd"] = keyMap["rsuper"];
        keyMap["cmd"] = keyMap["super"];

        keyMap["lalt"] = keyArray.add(SightKey(GLFW_KEY_LEFT_ALT));
        keyMap["ralt"] = keyArray.add(SightKey(GLFW_KEY_RIGHT_ALT));
        keyMap["alt"] = doubleKeyArray.add(SightTwoKey(keyMap["lalt"], keyMap["ralt"], false));

        keyMap["lshift"] = keyArray.add(SightKey(GLFW_KEY_LEFT_SHIFT));
        keyMap["rshift"] = keyArray.add(SightKey(GLFW_KEY_RIGHT_SHIFT));
        keyMap["shift"] = doubleKeyArray.add(SightTwoKey(keyMap["lshift"], keyMap["rshift"], false));

        // alphabet
        keyMap["a"] = keyArray.add(SightKey(GLFW_KEY_A));
        keyMap["b"] = keyArray.add(SightKey(GLFW_KEY_B));
        keyMap["c"] = keyArray.add(SightKey(GLFW_KEY_C));
        keyMap["d"] = keyArray.add(SightKey(GLFW_KEY_D));
        keyMap["e"] = keyArray.add(SightKey(GLFW_KEY_E));
        keyMap["f"] = keyArray.add(SightKey(GLFW_KEY_F));
        keyMap["g"] = keyArray.add(SightKey(GLFW_KEY_G));
        keyMap["h"] = keyArray.add(SightKey(GLFW_KEY_H));
        keyMap["i"] = keyArray.add(SightKey(GLFW_KEY_I));
        keyMap["j"] = keyArray.add(SightKey(GLFW_KEY_J));
        keyMap["k"] = keyArray.add(SightKey(GLFW_KEY_K));
        keyMap["l"] = keyArray.add(SightKey(GLFW_KEY_L));
        keyMap["m"] = keyArray.add(SightKey(GLFW_KEY_M));
        keyMap["n"] = keyArray.add(SightKey(GLFW_KEY_N));
        keyMap["o"] = keyArray.add(SightKey(GLFW_KEY_O));
        keyMap["p"] = keyArray.add(SightKey(GLFW_KEY_P));
        keyMap["q"] = keyArray.add(SightKey(GLFW_KEY_Q));
        keyMap["r"] = keyArray.add(SightKey(GLFW_KEY_R));
        keyMap["s"] = keyArray.add(SightKey(GLFW_KEY_S));
        keyMap["t"] = keyArray.add(SightKey(GLFW_KEY_T));
        keyMap["u"] = keyArray.add(SightKey(GLFW_KEY_U));
        keyMap["v"] = keyArray.add(SightKey(GLFW_KEY_V));
        keyMap["w"] = keyArray.add(SightKey(GLFW_KEY_W));
        keyMap["x"] = keyArray.add(SightKey(GLFW_KEY_X));
        keyMap["y"] = keyArray.add(SightKey(GLFW_KEY_Y));
        keyMap["z"] = keyArray.add(SightKey(GLFW_KEY_Z));

        // fx
        keyMap["f1"] = keyArray.add(SightKey(GLFW_KEY_F1));
        keyMap["f2"] = keyArray.add(SightKey(GLFW_KEY_F2));
        keyMap["f3"] = keyArray.add(SightKey(GLFW_KEY_F3));
        keyMap["f4"] = keyArray.add(SightKey(GLFW_KEY_F4));
        keyMap["f5"] = keyArray.add(SightKey(GLFW_KEY_F5));
        keyMap["f6"] = keyArray.add(SightKey(GLFW_KEY_F6));
        keyMap["f7"] = keyArray.add(SightKey(GLFW_KEY_F7));
        keyMap["f8"] = keyArray.add(SightKey(GLFW_KEY_F8));
        keyMap["f9"] = keyArray.add(SightKey(GLFW_KEY_F9));
        keyMap["f10"] = keyArray.add(SightKey(GLFW_KEY_F10));
        keyMap["f11"] = keyArray.add(SightKey(GLFW_KEY_F11));
        keyMap["f12"] = keyArray.add(SightKey(GLFW_KEY_F12));

        // up down left righ
        keyMap["up"] = keyArray.add(SightKey(GLFW_KEY_UP));
        keyMap["down"] = keyArray.add(SightKey(GLFW_KEY_DOWN));
        keyMap["left"] = keyArray.add(SightKey(GLFW_KEY_LEFT));
        keyMap["right"] = keyArray.add(SightKey(GLFW_KEY_RIGHT));

        // numbers
        keyMap["@1"] = keyArray.add(SightKey(GLFW_KEY_KP_1));
        keyMap["@2"] = keyArray.add(SightKey(GLFW_KEY_KP_2));
        keyMap["@3"] = keyArray.add(SightKey(GLFW_KEY_KP_3));
        keyMap["@4"] = keyArray.add(SightKey(GLFW_KEY_KP_4));
        keyMap["@5"] = keyArray.add(SightKey(GLFW_KEY_KP_5));
        keyMap["@6"] = keyArray.add(SightKey(GLFW_KEY_KP_6));
        keyMap["@7"] = keyArray.add(SightKey(GLFW_KEY_KP_7));
        keyMap["@8"] = keyArray.add(SightKey(GLFW_KEY_KP_8));
        keyMap["@9"] = keyArray.add(SightKey(GLFW_KEY_KP_9));
        keyMap["1"] = keyArray.add(SightKey(GLFW_KEY_1));
        keyMap["2"] = keyArray.add(SightKey(GLFW_KEY_2));
        keyMap["3"] = keyArray.add(SightKey(GLFW_KEY_3));
        keyMap["4"] = keyArray.add(SightKey(GLFW_KEY_4));
        keyMap["5"] = keyArray.add(SightKey(GLFW_KEY_5));
        keyMap["6"] = keyArray.add(SightKey(GLFW_KEY_6));
        keyMap["7"] = keyArray.add(SightKey(GLFW_KEY_7));
        keyMap["8"] = keyArray.add(SightKey(GLFW_KEY_8));
        keyMap["9"] = keyArray.add(SightKey(GLFW_KEY_9));
        keyMap["0"] = keyArray.add(SightKey(GLFW_KEY_0));

        // others
        keyMap["esc"] = keyArray.add(SightKey(GLFW_KEY_ESCAPE));
        keyMap["backspace"] = keyArray.add(SightKey(GLFW_KEY_BACKSPACE));
    }
}