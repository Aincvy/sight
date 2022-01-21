#include "sight_external_widgets.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cmath>
#include <iostream>

#include "sight_defines.h"


namespace sight {

    // imgui combo filter v1.0, by @r-lyeh (public domain)
    // contains code by @harold-b (public domain?)

    /*  Demo: */
    /*
    {
        // requisite: hints must be alphabetically sorted beforehand
        const char *hints[] = {
            "AnimGraphNode_CopyBone",
            "ce skipaa",
            "ce skipscreen",
            "ce skipsplash",
            "ce skipsplashscreen",
            "client_unit.cpp",
            "letrograd",
            "level",
            "leveler",
            "MacroCallback.cpp",
            "Miskatonic university",
            "MockAI.h",
            "MockGameplayTasks.h",
            "MovieSceneColorTrack.cpp",
            "r.maxfps",
            "r.maxsteadyfps",
            "reboot",
            "rescale",
            "reset",
            "resource",
            "restart",
            "retrocomputer",
            "retrograd",
            "return",
            "slomo 10",
            "SVisualLoggerLogsList.h",
            "The Black Knight",
        };
        static ComboFilterState s = {0};
        static char buf[128] = "type text here...";
        if( ComboFilter("my combofilter", buf, IM_ARRAYSIZE(buf), hints, IM_ARRAYSIZE(hints), s) ) {
            puts( buf );
        }
    }
*/

    static bool ComboFilter__DrawPopup(ComboFilterState& state, int START, std::vector<std::string> ENTRIES) {
        using namespace ImGui;
        bool clicked = 0;

        // Grab the position for the popup
        ImVec2 pos = GetItemRectMin();
        pos.y += GetItemRectSize().y;
        ImVec2 size = ImVec2(GetItemRectSize().x - 60, GetTextLineHeightWithSpacing() * 4);

        PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_HorizontalScrollbar |
            ImGuiWindowFlags_NoSavedSettings |
            0;     //ImGuiWindowFlags_ShowBorders;

        SetNextWindowFocus();

        SetNextWindowPos(pos);
        SetNextWindowSize(size);
        Begin("##combo_filter", nullptr, flags);

        PushAllowKeyboardFocus(false);

        for (int i = 0; i < ENTRIES.size(); i++) {
            // Track if we're drawing the active index so we
            // can scroll to it if it has changed
            bool isIndexActive = state.activeIdx == i;

            if (isIndexActive) {
                // Draw the currently 'active' item differently
                // ( used appropriate colors for your own style )
                PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 0, 1));
            }

            PushID(i);
            if (Selectable(ENTRIES[i].c_str(), isIndexActive)) {
                // And item was clicked, notify the input
                // callback so that it can modify the input buffer
                state.activeIdx = i;
                clicked = 1;
            }
            if (IsItemFocused() && IsKeyPressed(GetIO().KeyMap[ImGuiKey_Enter])) {
                // Allow ENTER key to select current highlighted item (w/ keyboard navigation)
                state.activeIdx = i;
                clicked = 1;
            }
            PopID();

            if (isIndexActive) {
                if (state.selectionChanged) {
                    // Make sure we bring the currently 'active' item into view.
                    // SetScrollHere();
                    SetScrollY(0);
                    state.selectionChanged = false;
                }

                PopStyleColor(1);
            }
        }

        PopAllowKeyboardFocus();
        End();
        PopStyleVar(1);

        return clicked;
    }

    bool ComboFilter(const char* id, char* buffer, int bufferlen, std::vector<std::string> hints, ComboFilterState& s) {
        struct fuzzy {
            static int score(const char* str1, const char* str2) {
                int score = 0, consecutive = 0, maxerrors = 0;
                while (*str1 && *str2) {
                    int is_leading = (*str1 & 64) && !(str1[1] & 64);
                    if ((*str1 & ~32) == (*str2 & ~32)) {
                        int had_separator = (str1[-1] <= 32);
                        int x = had_separator || is_leading ? 10 : consecutive * 5;
                        consecutive = 1;
                        score += x;
                        ++str2;
                    } else {
                        int x = -1, y = is_leading * -3;
                        consecutive = 0;
                        score += x;
                        maxerrors += y;
                    }
                    ++str1;
                }
                return score + (maxerrors < -9 ? -9 : maxerrors);
            }
            static int search(const char* str, int num, std::vector<std::string> words) {
                int scoremax = 0;
                int best = -1;
                for (int i = 0; i < num; ++i) {
                    int score = fuzzy::score(words[i].c_str(), str);
                    int record = (score >= scoremax);
                    int draw = (score == scoremax);
                    if (record) {
                        scoremax = score;
                        if (!draw)
                            best = i;
                        else
                            best = best >= 0 && words[best].length() < words[i].length() ? best : i;
                    }
                }
                return best;
            }
        };
        auto num_hints = hints.size();
        using namespace ImGui;
        bool done = InputText(id, buffer, bufferlen, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
        bool hot = s.activeIdx >= 0 && strcmp(buffer, hints[s.activeIdx].c_str());
        if (hot) {
            int new_idx = fuzzy::search(buffer, num_hints, hints);
            int idx = new_idx >= 0 ? new_idx : s.activeIdx;
            s.selectionChanged = s.activeIdx != idx;
            s.activeIdx = idx;
            if (done || ComboFilter__DrawPopup(s, idx, hints)) {
                int i = s.activeIdx;
                if (i >= 0) {
                    strcpy(buffer, hints[i].c_str());
                    done = true;
                }
            }
        }
        return done;
    }

    void ToggleButton(const char* str_id, bool* v) {
        ImVec4* colors = ImGui::GetStyle().Colors;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight();
        float width = height * 1.55f;
        float radius = height * 0.50f;

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if (ImGui::IsItemClicked())
            *v = !*v;
        ImGuiContext& gg = *GImGui;
        float ANIM_SPEED = 0.085f;
        if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id))     // && g.LastActiveIdTimer < ANIM_SPEED)
            float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
        else
            draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * 0.50f);
        draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
    }

    std::ostream& operator<<(std::ostream& out, const ImVec2& v) {
        out << v.x << ", " << v.y;
        return out;
    }
    
}

namespace ImGui {

    bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size = size_arg;
        size.x -= style.FramePadding.x * 2;

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;

        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd = size.x;
        const float circleWidth = circleEnd - circleStart;

        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

        const float t = g.Time;
        const float r = size.y / 2;
        const float speed = 1.5f;

        const float a = speed * 0;
        const float b = speed * 0.333f;
        const float c = speed * 0.666f;

        const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
        const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
        const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
        return true;
    }

    bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;

        // Render
        window->DrawList->PathClear();

        int num_segments = 30;
        int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
            window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
                                                centre.y + ImSin(a + g.Time * 8) * radius));
        }

        window->DrawList->PathStroke(color, false, thickness);
        return true;
    }

    void LoadingIndicatorCircle(const char* label, const float indicator_radius,
                                       const ImVec4& main_color, const ImVec4& backdrop_color,
                                       const int circle_count, const float speed) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) {
            return;
        }

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(label);

        const ImVec2 pos = window->DC.CursorPos;
        const float circle_radius = indicator_radius / 10.0f;
        const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f,
                                    pos.y + indicator_radius * 2.0f));

        ItemSize(bb, ImGui::GetStyle().FramePadding.y);
        if (!ItemAdd(bb, id)) {
            return;
        }
        const float t = g.Time;
        const auto degree_offset = 2.0f * IM_PI / circle_count;
        for (int i = 0; i < circle_count; ++i) {
            const auto x = indicator_radius * std::sin(degree_offset * i);
            const auto y = indicator_radius * std::cos(degree_offset * i);
            const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
            ImVec4 color;
            color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
            color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
            color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
            color.w = 1.0f;
            window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x,
                                                     pos.y + indicator_radius - y),
                                              circle_radius + growth * circle_radius,
                                              GetColorU32(color));
        }
    }
}