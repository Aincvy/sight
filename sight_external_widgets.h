#pragma once

#include "imgui.h"
#include <vector>
#include <string>

namespace sight {

    struct ComboFilterState {
        int activeIdx;             // Index of currently 'active' item by use of up/down keys
        bool selectionChanged;     // Flag to help focus the correct item when selecting active item
    };


    /**
     * @brief 
     * Copied from https://github.com/ocornut/imgui/issues/1658#issuecomment-427426154.
     * It has some questions about focused.
     * @param id 
     * @param buffer 
     * @param bufferlen 
     * @param hints 
     * @param s 
     * @return true 
     * @return false 
     */
    bool ComboFilter(const char* id, char* buffer, int bufferlen, std::vector<std::string> hints, ComboFilterState& s);

    /**
     * @brief copy from https://github.com/ocornut/imgui/issues/1537#issuecomment-780262461
     * 
     * @param str_id 
     * @param v 
     */
    void ToggleButton(const char* str_id, bool* v);

    std::ostream& operator<<(std::ostream& out, const ImVec2& v);
    
}

namespace ImGui {

    /**
     * @brief 
     * copy from https://github.com/ocornut/imgui/issues/1901#issue-335266223 
     *
     * @param label 
     * @param value 
     * @param size_arg 
     * @param bg_col 
     * @param fg_col 
     * @return true 
     * @return false 
     */
    bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);

    /**
     * @brief 
     * copy from https://github.com/ocornut/imgui/issues/1901#issue-335266223  
     *
     * @param label 
     * @param radius 
     * @param thickness 
     * @param color 
     * @return true 
     * @return false 
     */
    bool Spinner(const char* label, float radius, int thickness, const ImU32& color);

    /**
     * @brief https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
     * 
     * @param label 
     * @param indicator_radius 
     * @param main_color 
     * @param backdrop_color 
     * @param circle_count 
     * @param speed 
     */
    void LoadingIndicatorCircle(const char* label, const float indicator_radius,
                                const ImVec4& main_color, const ImVec4& backdrop_color,
                                const int circle_count, const float speed);
}   