#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <atomic>
#include <imgui.h>
#include <thread>

namespace vanta::ui {

static const char* kStandardHotkeys[] = {
    "panel.toggle",
    "panic.kill",
    "aim.engage",
    "trigger.engage",
    "bhop.engage",
};

void draw_hotkeys(App& app) {
    ImGui::TextUnformatted("Hotkeys");
    ImGui::TextDisabled("Click 'Bind' and press any key (with modifiers held).");

    auto& map = app.hotkeys();
    static std::string capturing;
    static std::atomic<bool> cancel{false};

    // Ensure all standard entries exist.
    for (auto name : kStandardHotkeys) {
        if (map.entries.find(name) == map.entries.end()) map.entries[name] = {};
    }

    if (ImGui::BeginTable("##hk", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("action");
        ImGui::TableSetupColumn("binding");
        ImGui::TableSetupColumn("actions");
        ImGui::TableHeadersRow();
        for (auto& [name, hk] : map.entries) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(name.c_str());
            ImGui::TableNextColumn();
            if (capturing == name) {
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Palette::accent), "press key...");
            } else {
                ImGui::TextUnformatted(hk.display().c_str());
                auto col = map.collisions(hk, name);
                if (!col.empty()) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImGui::ColorConvertU32ToFloat4(Palette::warning));
                    ImGui::Text("(collision with %s)", col.front().c_str());
                    ImGui::PopStyleColor();
                }
            }
            ImGui::TableNextColumn();
            ImGui::PushID(name.c_str());
            if (capturing == name) {
                if (ImGui::SmallButton("Cancel")) { cancel.store(true); capturing.clear(); }
            } else {
                if (ImGui::SmallButton("Bind")) {
                    capturing = name;
                    cancel.store(false);
                    std::thread([name, &map]() {
                        auto h = input::capture_blocking([]{ return cancel.load(); });
                        if (h.vk != 0) {
                            map.entries[name] = h;
                            map.save_to_settings();
                        }
                        capturing.clear();
                    }).detach();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Clear")) {
                    map.entries[name] = {};
                    map.save_to_settings();
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    section_header("PANIC");
    ImGui::TextWrapped(
        "The 'panic.kill' hotkey immediately disables ALL injected input (mouse "
        "move, mouse click, key tap) until Dashboard → 'Clear PANIC' is pressed. "
        "Nothing in Vanta can override it.");
}

} // namespace vanta::ui
