#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"
#include "core/Config.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_profiles(App& app) {
    ImGui::TextUnformatted("Profiles");
    ImGui::TextDisabled("Named snapshots of every setting; swap between loadouts.");

    static char n_name[64] = "new-profile";
    static std::string pending_delete;

    section_header("ACTIVE");
    ImGui::Text("current: %s", app.profiles().active().c_str());
    if (ImGui::Button("Save over active")) {
        app.profiles().save_active(app.profiles().active());
    }

    section_header("MANAGE");
    ImGui::InputText("name", n_name, sizeof(n_name));
    ImGui::SameLine();
    if (ImGui::Button("Create")) {
        app.profiles().create(n_name, config::Settings::instance().snapshot());
    }

    auto list = app.profiles().list();
    if (ImGui::BeginTable("##prof", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("name");
        ImGui::TableSetupColumn("actions");
        ImGui::TableHeadersRow();
        for (auto& p : list) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.c_str());
            ImGui::TableNextColumn();
            ImGui::PushID(p.c_str());
            if (ImGui::SmallButton("Activate")) app.profiles().activate(p);
            ImGui::SameLine();
            if (ImGui::SmallButton("Duplicate")) app.profiles().duplicate(p, p + "-copy");
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) {
                pending_delete = p;
                ImGui::OpenPopup("confirm-delete");
            }
            if (ImGui::BeginPopupModal("confirm-delete", nullptr,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete profile '%s'? This cannot be undone.", pending_delete.c_str());
                if (ImGui::Button("Delete")) {
                    app.profiles().remove(pending_delete);
                    pending_delete.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    pending_delete.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    section_header("DEFAULTS");
    if (ImGui::Button("Restore defaults for active profile")) {
        app.profiles().restore_defaults();
    }
}

} // namespace vanta::ui
