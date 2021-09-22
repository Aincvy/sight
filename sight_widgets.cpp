#include "sight_widgets.h"
#include "sight.h"
#include "sight_defines.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdio>

namespace sight {

    bool Selectable(int id, const char* text, bool selected, ImU32 color) {
        char labelBuf[LITTLE_NAME_BUF_SIZE] = {0};
        sprintf(labelBuf, "## %d", id);
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        auto pos = window->DC.CursorPos;
        auto size = ImGui::CalcTextSize(text);

        auto r = ImGui::Selectable(labelBuf, selected, 0, size);
        window->DrawList->AddText(pos, color, text);
        return r;
    }

    

}