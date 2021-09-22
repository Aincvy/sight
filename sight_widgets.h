
#pragma once

#include "sight_util.h"
#include "imgui.h"

namespace sight {

    bool Selectable(int id, const char* text, bool selected = false, ImU32 color = IM_COL32(255,255,255,255));

}