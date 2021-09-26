
#pragma once

#include "imgui.h"
#include "sight_util.h"

namespace sight {

    struct SightImage {
        ImTextureID textureId;
        int width = 0;
        int height = 0;

        bool ready() const;
    };

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
    std::string openFileDialog(const char* basePath);

    /**
     * @brief 
     * powered by https://github.com/btzy/nativefiledialog-extended
     * 
     * @param basePath 
     * @return std::string 
     */
    std::string saveFileDialog(const char* basePath);

    
}