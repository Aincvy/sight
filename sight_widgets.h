
#pragma once

#include "imgui.h"
#include "sight_util.h"
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>


namespace sight {

    struct SightImage {
        ImTextureID textureId;
        int width = 0;
        int height = 0;

        bool ready() const;
    };

    class SightEmptyKey {
    public:
        virtual bool isKeyReleased() const;
        virtual bool isKeyDown() const;
        // same as isKeyReleased()
        virtual bool isKeyUp() const;
        virtual bool IsKeyPressed(bool repeat = false) const;

        SightEmptyKey() = default;
        virtual ~SightEmptyKey();

        /**
         * @brief is key down ?
         * 
         * @return true 
         * @return false 
         */
        virtual operator bool() const;
    };

    class SightKey : public SightEmptyKey {
        ushort code{0};

    public:
        SightKey() = default;
        SightKey(SightKey const& key) = default;
        SightKey(ushort code);
        ~SightKey() = default;

        bool isKeyReleased() const override;
        bool isKeyDown() const override;
        bool isKeyUp() const override;
        bool IsKeyPressed(bool repeat = false) const override;
    };

    class SightTwoKey : public SightEmptyKey {
        SightEmptyKey* key1{nullptr};
        SightEmptyKey* key2{nullptr};
        // if both = true, then need key1 and key2 all has the status
        bool both{true};

    public:
        SightTwoKey() = default;
        SightTwoKey(SightTwoKey const& key);
        SightTwoKey(SightEmptyKey* key1, SightEmptyKey* key2);
        SightTwoKey(SightEmptyKey* key1, SightEmptyKey* key2, bool both);
        ~SightTwoKey() = default;

        bool isKeyReleased() const override;
        bool isKeyDown() const override;
        bool IsKeyPressed(bool repeat = false) const override;
    };

    class SightKeyWrapper {
        SightEmptyKey* pointer = nullptr; 
        std::string tips;
        
    public:
        SightKeyWrapper(SightEmptyKey* pointer);
        SightKeyWrapper() =default;
        ~SightKeyWrapper() = default;

        bool isKeyReleased() const;
        bool isKeyDown() const;
        bool isKeyUp() const;
        bool IsKeyPressed(bool repeat = false) const;

        operator bool();

        const char* tipsText() const;
        std::string const& getTips() const;
        void setTips(std::string tips);
    };

    enum class ToastType {
        Info,
        Warning,
        Error,
    };

    /**
     * @brief Toast widget.
     * 
     */
    struct ToastElement{
        uint id = 0;
        // cache string for imgui window title.
        std::string windowTitle;
        // content title.
        std::string title;
        std::string content;
        // target time, -1 = do not disappear.
        float disappearTime = 0;
        bool close = false;
        ToastType toastType = ToastType::Info;

        ToastElement() = default;
        ToastElement(ToastElement const&) = default;
        ~ToastElement() = default;
        ToastElement(uint id, std::string title, std::string content, float disappearTime);

        void render(ImVec2 const& pos, ImVec2 const& size);
        ImVec2 calcRecommendSize() const;

        bool isClosed() const;
        void closeToast();
    };

    /**
     * @brief A controller 
     * 
     */
    struct ToastController {
        std::vector<ToastElement> elements;

        // next toast args.
        ToastType nextToastType = ToastType::Info;


        ToastController& info();
        ToastController& warning();
        ToastController& error();

        bool toast(std::string_view title, std::string_view content, float showTime = 5);

        void render();
        void resetNextArgs();
    };

    /**
     * @brief init keyboard keys
     * 
     */
    void initKeys();

    /**
     * @brief Get the From Key Map object
     * 
     * @param str 
     * @return SightEmptyKey* 
     */
    SightEmptyKey* getFromKeyMap(std::string str);

    /**
     * @brief Get the Two Key From String object
     * 
     * @param str split use `+`
     * @return SightTwoKey* 
     */
    SightTwoKey* getTwoKeyFromString(std::string const& str);

    /**
     * @brief 
     * 
     * @param path 
     * @param height 
     * @param weight 
     * @return ImTextureID 
     */
    ImTextureID loadImage(const char* path, int* height, int* weight);

    /**
     * @brief 
     * 
     * @param path 
     * @param image 
     * @return true 
     * @return false 
     */
    bool loadImage(const char* path, SightImage* image);


    bool Selectable(int id, const char* text, bool selected = false, ImU32 color = IM_COL32(255, 255, 255, 255));

    /**
     * @brief use os native file dialog. 
     * powered by https://github.com/btzy/nativefiledialog-extended
     * 
     * @param basePath 
     * @return std::string 
     */
    std::string openFileDialog(const char* basePath, int* status, std::vector<std::pair<const char*, const char*>> filter);
    std::string openFileDialog(const char* basePath, int* status);

    /**
     * @brief 
     * powered by https://github.com/btzy/nativefiledialog-extended
     * 
     * @param basePath 
     * @return std::string 
     */
    std::string openFolderDialog(const char* basePath, int* status);

    /**
     * @brief 
     * powered by https://github.com/btzy/nativefiledialog-extended
     * 
     * @param basePath 
     * @return std::string 
     */
    std::string saveFileDialog(const char* basePath, int* status = nullptr, std::string_view defaultName = "");

    void openUrlWithDefaultBrowser(std::string const& urlString);

    ImU32 randomColor(bool randomAlpha = false);

    /**
     * @brief 
     * copied by imgui_demo.cpp
     * 
     * @param desc 
     */
    void helpMarker(const char* desc);

    bool checkBox(const char* label, bool *v, bool readonly = false);

    /**
     * @brief 
     * 
     * @param allowEnter   true: key `enter` also trigger 
     * @return true 
     * @return false 
     */
    inline bool buttonOK(const char* label = "OK", bool allowEnter = true) {
        return ImGui::Button(label) || (allowEnter && ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Enter)));
    }

    inline ImVec4 rgba(int r, int g, int b, int a) {
        return ImVec4{ static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f, static_cast<float>(a) / 255.f };
    }

    inline ImVec4 rgb(int r, int g, int b) {
        return rgba(r, g, b, 255);
    }

    template<class... Args>
    void leftText(const char* format, Args&&... args) {
        ImGui::Text(format, args...);
        ImGui::SameLine();
    }
    
}