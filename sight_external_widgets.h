#pragma once

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
     * Do not use for now.
     * @param id 
     * @param buffer 
     * @param bufferlen 
     * @param hints 
     * @param s 
     * @return true 
     * @return false 
     */
    bool ComboFilter(const char* id, char* buffer, int bufferlen, std::vector<std::string> hints, ComboFilterState& s);

}