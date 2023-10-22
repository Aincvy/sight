#include "sight_widgets.h"
#include "IconsMaterialDesign.h"
#include "nfd.h"
#include "sight.h"
#include "sight_defines.h"
#include "sight_memory.h"
#include "sight_undo.h"
#include "sight_util.h"
#include "sight_colors.h"
#include "sight_log.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "nfd.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_split.h"

#ifdef OPENGL
#    include "GLFW/glfw3.h"
#endif

// #ifdef __APPLE__
// #include <CoreFoundation/CFBundle.h>
// #include <ApplicationServices/ApplicationServices.h>
// #endif

namespace sight {

    namespace {
        SightArray<SightKey> keyArray(LITTLE_ARRAY_SIZE);
        SightArray<SightTwoKey> doubleKeyArray(LITTLE_ARRAY_SIZE);

        absl::flat_hash_map<std::string, SightEmptyKey*> keyMap(LITTLE_ARRAY_SIZE);
    }

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

        #ifdef OPENGL
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
        #endif 

        return nullptr;
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
    
    std::string openFileDialog(const char* basePath, int* status){
        return openFileDialog(basePath, status, {});
    }

    std::string openFileDialog(const char* basePath, int* status, std::vector<std::pair<const char*, const char*>> filter) {
        // todo 实现并修正这个函数
        // // initialize NFD
        // NFD::Guard nfdGuard;
        // // auto-freeing memory
        // NFD::UniquePath outPath;

        // // prepare filters for the dialog
        // std::pair<const char*, const char*> a = {"", ""};
        // auto size = std::size(filter);
        // nfdnfilteritem_t data[size];
        // if (size > 0) {
        //     int i = -1;
        //     for (const auto& item : filter) {
        //         i++;
        //         data[i] = {.name = item.first, .spec = item.second};
        //     }
        // }

        // // show the dialog
        // std::string pathResult{};

        // nfdresult_t result = NFD::OpenDialog(outPath, data, size, basePath);
        // if (result == NFD_OKAY) {
        //     SET_CODE(status, CODE_OK);
        //     pathResult = outPath.get();
        // } else if (result == NFD_CANCEL) {
        //     // std::cout << "User pressed cancel." << std::endl;
        //     SET_CODE(status, CODE_USER_CANCELED);
        // } else {
        //     std::cout << "Error: " << NFD::GetError() << std::endl;
        //     SET_CODE(status, CODE_ERROR);
        // }
        // // NFD::Guard will automatically quit NFD.

        // return pathResult;

        return {};
    }

    std::string openFolderDialog(std::string_view basePath, int* status) {
        // initialize NFD
        NFD::Guard nfdGuard;
        // auto-freeing memory
        NFD::UniquePath outPath;

        
#ifdef _WIN32
        const char* defaultPath = nullptr;
        if (basePath.length() >= 1 && basePath[0] != '.') {
            defaultPath = basePath.data();
        }
#else
        const char* defaultPath = basePath.data();
#endif
        logDebug("openFolderDialog defaultPath: $0", defaultPath);
        nfdresult_t result = NFD::PickFolder(outPath, defaultPath);
        if (result == NFD_OKAY) {
            SET_CODE(status, CODE_OK);
            return outPath.get();
        } else if (result == NFD_CANCEL) {
            SET_CODE(status, CODE_USER_CANCELED);
        } else {
            // std::cout << "Error: " << NFD::GetError() << std::endl;
            logError(NFD::GetError());
            SET_CODE(status, CODE_ERROR);
        }

        return {};
    }

    std::string saveFileDialog(const char* basePath, int* status, std::string_view defaultName) {
        // initialize NFD
        NFD::Guard nfdGuard;
        // auto-freeing memory
        NFD::UniquePath outPath;

        auto result = NFD::SaveDialog(outPath, nullptr, 0, basePath, defaultName.data());
        if (result == NFD_OKAY) {
            SET_CODE(status, CODE_OK);
            return outPath.get();
        } else if (result == NFD_CANCEL) {
            SET_CODE(status, CODE_USER_CANCELED)
        } else {
            SET_CODE(status, CODE_ERROR);
        }

        return {};
    }

    void openUrlWithDefaultBrowser(std::string const& urlString) {
#ifdef __APPLE__
        // CFURLRef url = CFURLCreateWithBytes(
        //     NULL,                          // allocator
        //     (UInt8*)urlString.c_str(),     // URLBytes
        //     urlString.length(),            // length
        //     kCFStringEncodingASCII,        // encoding
        //     NULL                           // baseURL
        // );
        // LSOpenCFURLRef(url, 0);
        // CFRelease(url);
        
        system(("open " + urlString).c_str());
#elif _WIN32
        system(("start " + urlString).c_str());
#endif
    }

    ImU32 randomColor(bool randomAlpha) {
        int r,g,b,a = 255;
        std::random_device device;
        std::mt19937 rng(device());
        std::uniform_int_distribution<std::mt19937::result_type> range(0,255);
        r = range(rng);
        g = range(rng);
        b = range(rng);
        if (randomAlpha) {
            a = range(rng);
        }
        // dbg(r,g,b,a);
        return IM_COL32(r,g,b,a);
    }

    void helpMarker(const char* desc) {
        // ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool checkBox(const char* label, bool *v, bool readonly) {
        if (readonly) {
            auto t = *v;
            ImGui::BeginDisabled();
            ImGui::Checkbox(label, &t);
            ImGui::EndDisabled();
            return false;
        } else {
            return ImGui::Checkbox(label, v);
        }
    }

    SightKey::SightKey(int code)
        : code(code) {
    }

    bool SightKey::isKeyReleased() const {
        logDebug(code);
        return ImGui::IsKeyReleased((ImGuiKey)code);
    }

    bool SightKey::isKeyDown() const {
        return ImGui::IsKeyDown((ImGuiKey)code);
    }

    bool SightKey::isKeyUp() const {
        return isKeyReleased();
    }

    bool SightKey::IsKeyPressed(bool repeat) const {
        return ImGui::IsKeyPressed((ImGuiKey)code, repeat);
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
        logDebug("1");
        if (this->both) {
            // ImGui::IsKeyPressed((ImGuiKey)1);     // why zheli you ge 1 ?
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

    ToastElement::ToastElement(uint id, std::string title, std::string content, float disappearTime)
        : id(id)
        , title(std::move(title))
        , disappearTime(disappearTime)
        , content(std::move(content)) {
            // window title
            windowTitle = "##toast-";
            absl::StrAppend(&windowTitle, id);
            logDebug(windowTitle.c_str());
    }

    void ToastElement::render(ImVec2 const& pos, ImVec2 const& size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        if (ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
            if (!title.empty()) {
                ImGui::Text("%s", title.c_str());
                
                ImGui::SameLine();
            }
            if (ImGui::Button(ICON_MD_CLOSE)) {
                closeToast();
            }
            if (!content.empty()) {
                ImGui::TextWrapped("%s", content.c_str());
            }
        }
        ImGui::End();
    }

    ImVec2 ToastElement::calcRecommendSize() const {
        constexpr int width = 280;
        const int height = ImGui::GetFrameHeight();
        return { width, 3.0f * height };
    }

    bool ToastElement::isClosed() const {
        return close || (disappearTime > 0 && ImGui::GetTime() > disappearTime);
    }

    void ToastElement::closeToast() {
        close = true;
    }

    ToastController& ToastController::info() {
        nextToastType = ToastType::Info;
        return *this;
    }

    ToastController& ToastController::warning() {
        nextToastType = ToastType::Warning;
        return *this;
    }

    ToastController& ToastController::error() {
        nextToastType = ToastType::Error;
        return *this;
    }

    bool ToastController::toast(std::string_view title, std::string_view content, float showTime) {
        auto now = ImGui::GetTime();
        elements.emplace_back(getRuntimeId(), title.data(), content.data(), showTime > 0 ? now + showTime : showTime);
        
        // apply next args..
        auto& element = elements.back();
        element.toastType = nextToastType;

        switch (element.toastType) {
        case ToastType::Info:
            logInfo("$0, $1", title, content);
            break;
        case ToastType::Warning:
            logWarning("$0, $1", title, content);
            break;
        case ToastType::Error:
            logError("$0, $1", title, content);
            break;
        }
        
        resetNextArgs();
        return true;
    }

    void ToastController::render() {
        // auto now = ImGui::GetTime();
        elements.erase(std::remove_if(elements.begin(), elements.end(), [](ToastElement const& e){ return e.isClosed(); }), elements.end());
        
        // do position. 
        constexpr int rightSpacing = 25;
        constexpr int bottomSpacing = 25;
        
        auto displaySize = ImGui::GetIO().DisplaySize;
        const auto windowWidth = displaySize.x;
        const auto windowHeight = displaySize.y;
        int x = windowWidth - 25;
        int y = windowHeight - bottomSpacing;
        for (auto iter = elements.rbegin(); iter != elements.rend(); iter++) {
            auto& item = *iter;

            auto size = item.calcRecommendSize();
            y -= size.y;
            item.render(ImVec2(x - size.x, y), size);

            // x += size.x;
            y -= bottomSpacing;
        }
        
    }

    void ToastController::resetNextArgs() {
        info();

    }

}