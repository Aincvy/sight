#include "sight_widgets.h"
#include "nfd.h"
#include "sight.h"
#include "sight_defines.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstddef>
#include <cstdio>

#include "GLFW/glfw3.h"
#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "nfd.hpp"

namespace sight {

    ImTextureID loadImage(const char* path, int* height, int* width) {
        auto data = stbi_load(path, width, height, nullptr, 4);
        if (data == nullptr) {
            return nullptr;
        }

        GLuint imageTexture;
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);     // This is required on WebGL for non power-of-two textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);     // Same

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

        return reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(imageTexture));
    }

    bool loadImage(const char* path, SightImage* image) {
        int w = 0, h = 0;
        auto p = loadImage(path, &h, &w);
        if (p == nullptr) {
            return false;
        }
        image->width = w;
        image->height = h;
        image->textureId = p;
        return true;
    }

    bool SightImage::ready() const {
        return height > 0 && width > 0;
    }


    bool Selectable(int id, const char* text, bool selected, ImU32 color) {
        char labelBuf[LITTLE_NAME_BUF_SIZE] = { 0 };
        sprintf(labelBuf, "## selectable.%d", id);
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        auto pos = window->DC.CursorPos;
        auto size = ImGui::CalcTextSize(text);

        auto r = ImGui::Selectable(labelBuf, selected, 0, size);
        window->DrawList->AddText(pos, color, text);
        return r;
    }

    std::string openFileDialog(const char* basePath) {
        // initialize NFD
        NFD::Guard nfdGuard;
        // auto-freeing memory
        NFD::UniquePath outPath;

        // prepare filters for the dialog
        nfdfilteritem_t filterItem[2] = { { "Source code", "c,cpp,cc" }, { "Graphs", "yaml" } };

        // show the dialog
        std::string pathResult{};
        
        nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 2, basePath);
        if (result == NFD_OKAY) {
            std::cout << "Success!" << std::endl
                      << outPath.get() << std::endl;
            pathResult = outPath.get();
        } else if (result == NFD_CANCEL) {
            std::cout << "User pressed cancel." << std::endl;
        } else {
            std::cout << "Error: " << NFD::GetError() << std::endl;
        }

        // NFD::Guard will automatically quit NFD.

        return pathResult;
    }

    std::string saveFileDialog(const char* basePath) {
        // initialize NFD
        NFD::Guard nfdGuard;
        // auto-freeing memory
        NFD::UniquePath outPath;

        auto result = NFD::SaveDialog(outPath, nullptr, 0, basePath, "graph");
        if (result == NFD_OKAY) {
            return outPath.get();
        }

        return {};
    }

}     