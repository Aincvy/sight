#include "sight_widgets.h"
#include "nfd.h"
#include "sight.h"
#include "sight_defines.h"
#include "sight_memory.h"
#include "sight_util.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "nfd.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_split.h"

namespace sight {

    namespace {
        SightArray<SightKey> keyArray(LITTLE_ARRAY_SIZE);
        SightArray<SightTwoKey> doubleKeyArray(LITTLE_ARRAY_SIZE);

        absl::flat_hash_map<std::string, SightEmptyKey*> keyMap(LITTLE_ARRAY_SIZE);
    }

    //////////////////////////////////////////////////////////////////////////////////
    //          Begin Of Keys
    //////////////////////////////////////////////////////////////////////////////////
    /* spellchecker: disable */
#ifdef __APPLE__
    
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

    }

#elif _WIN32
    // windows key
    void initKeys() {
    }
#endif

    /* spell-checker: enable */
    //////////////////////////////////////////////////////////////////////////////////
    //          End Of Keys
    //////////////////////////////////////////////////////////////////////////////////

    SightEmptyKey* getFromKeyMap(std::string str) {
        lowerCase(str);
        auto it = keyMap.find(str);
        if (it != keyMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    SightTwoKey* getTwoKeyFromString(std::string const& str) {
        std::vector<std::string> strings = absl::StrSplit(str, '+');
        if (strings.size() == 2) {
            auto key1 = getFromKeyMap(strings[0]);
            auto key2 = getFromKeyMap(strings[1]);
            return doubleKeyArray.add(SightTwoKey(key1, key2));
        }
        return nullptr;
    }


    ImTextureID
    loadImage(const char* path, int* height, int* width) {
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

    SightKey::SightKey(ushort code)
        : code(code) {
    }

    bool SightKey::isKeyReleased() const {
        dbg(code);
        return ImGui::IsKeyReleased(code);
    }

    bool SightKey::isKeyDown() const {
        return ImGui::IsKeyDown(code);
    }

    bool SightKey::isKeyUp() const {
        return isKeyReleased();
    }

    bool SightKey::IsKeyPressed(bool repeat) const {
        return ImGui::IsKeyPressed(code, repeat);
    }

    bool SightEmptyKey::isKeyReleased() const {
        return false;
    }

    bool SightEmptyKey::isKeyDown() const {
        return false;
    }

    bool SightEmptyKey::isKeyUp() const {
        return isKeyReleased();
    }

    bool SightEmptyKey::IsKeyPressed(bool repeat) const {
        return false;
    }

    SightEmptyKey::operator bool() const {
        // dbg("SightEmptyKey bool");
        return IsKeyPressed();
    }

    SightEmptyKey::~SightEmptyKey() {
    }

    SightTwoKey::SightTwoKey(SightTwoKey const& key)
        : key1(key.key1)
        , key2(key.key2)
        , both(key.both) {
    }

    SightTwoKey::SightTwoKey(SightEmptyKey* key1, SightEmptyKey* key2)
        : SightTwoKey(key1, key2, true) {
    }

    SightTwoKey::SightTwoKey(SightEmptyKey* key1, SightEmptyKey* key2, bool both)
        : key1(key1)
        , key2(key2)
        , both(both) {

    }

    bool SightTwoKey::isKeyReleased() const {
        dbg("1");
        if (this->both) {
            ImGui::IsKeyPressed(1);
            return key1->isKeyReleased() && key2->isKeyReleased();
        }
        return key1->isKeyReleased() || key2->isKeyReleased();
    }


    bool SightTwoKey::isKeyDown() const {
        if (this->both) {
            return key1->isKeyDown() && key2->isKeyDown();
        }
        return key1->isKeyDown() || key2->isKeyDown();
    }

    bool SightTwoKey::IsKeyPressed(bool repeat) const {
        if (this->both) {
            return key1->IsKeyPressed(repeat) && key2->IsKeyPressed(repeat);
        }

        return key1->IsKeyPressed(repeat) || key2->IsKeyPressed(repeat);
    }

    SightKeyWrapper::SightKeyWrapper(SightEmptyKey* pointer) : pointer(pointer)
    {
        
    }

    bool SightKeyWrapper::isKeyReleased() const {
        return pointer && pointer->isKeyReleased();
    }

    bool SightKeyWrapper::isKeyDown() const {
        return pointer && pointer->isKeyDown();
    }

    bool SightKeyWrapper::isKeyUp() const {
        return pointer && pointer->isKeyUp();
    }

    std::string const& SightKeyWrapper::getTips() const {
        return tips;
    }

    void SightKeyWrapper::setTips(std::string tips) {
        this->tips = std::move(tips);
    }

    bool SightKeyWrapper::IsKeyPressed(bool repeat) const {
        return pointer && pointer->IsKeyPressed(repeat);
    }

    SightKeyWrapper::operator bool() {
        return pointer && (*pointer);
    }

    const char* SightKeyWrapper::tipsText() const {
        if (this->getTips().empty()) {
            return nullptr;
        }
        return this->getTips().c_str();
    }

}