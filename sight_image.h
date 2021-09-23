#pragma once

#include "imgui.h"

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

}