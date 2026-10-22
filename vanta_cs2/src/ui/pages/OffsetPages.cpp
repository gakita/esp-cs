#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"
#include "core/AtomicFile.hpp"
#include "core/FilePaths.hpp"

#include <chrono>
#include <imgui.h>
#include <nlohmann/json.hpp>

namespace vanta::ui {

static ImU32 compat_color(offsets::Compat c) {
    switch (c) {
        case offsets::Compat::Confirmed: return Palette::success;
        case offsets::Compat::Unknown:   return Palette::text_dim;
        case offsets::Compat::Suspect:   return Palette::warning;
        case offsets::Compat::Stale:     return Palette::warning;
        case offsets::Compat::Invalid:   return Palette::error;
    }
    return Palette::text_dim;
}

void draw_offset_manager(App& app) {
    ImGui::TextUnformatted("Offset Manager");
    ImGui::TextDisabled("Promote fetched bundles to active, verify fields, compare builds.");

    section_header("ACTIVE BUNDLE");
    auto active = app.offsets().active_cached();
    if (!active) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Palette::warning),
                           "No active bundle. Fetch one on the Source Manager page.");
        return;
    }
    ImGui::Text("source:  %s", active->source_name.c_str());
    ImGui::Text("version: %s", active->source_version.c_str());
    ImGui::Text("build:   %s", active->game_build.c_str());
    ImGui::Text("sha256:  %s", active->sha256_hex.c_str());
    status_dot(compat_color(active->overall()),
               offsets::compat_label(active->overall()));

    section_header("FIELDS");
    if (ImGui::BeginTable("##fields", 5,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("name");
        ImGui::TableSetupColumn("value");
        ImGui::TableSetupColumn("base");
        ImGui::TableSetupColumn("compat");
        ImGui::TableSetupColumn("evidence");
        ImGui::TableHeadersRow();
        for (auto& f : active->fields) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(f.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("0x%llX", (unsigned long long)f.value);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(f.base.c_str());
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(compat_color(f.compat)));
            ImGui::TextUnformatted(offsets::compat_label(f.compat));
            ImGui::PopStyleColor();
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", f.evidence.c_str());
        }
        ImGui::EndTable();
    }

    section_header("ACTIONS");
    if (ImGui::Button("Export active to file...")) {
        auto path = paths::app_data() / L"exported_offsets.json";
        io::write_atomic(path, active->to_json().dump(2));
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload from disk")) {
        app.offsets().refresh_active_from_disk();
    }
}

void draw_offset_catalog(App& app) {
    ImGui::TextUnformatted("Offset Catalog");
    ImGui::TextDisabled("Required field names Vanta looks up in whatever active bundle you adopt.");

    static const char* required_fields[] = {
        "dwLocalPlayerPawn",
        "dwLocalPlayerController",
        "dwEntityList",
        "dwViewMatrix",
        "dwPlantedC4",
        "C_BaseEntity::m_iTeamNum",
        "C_BaseEntity::m_iHealth",
        "C_BaseEntity::m_pGameSceneNode",
        "CGameSceneNode::m_vecAbsOrigin",
        "CGameSceneNode::m_modelState",
        "CModelState::m_hModel",
        "C_CSPlayerPawnBase::m_ArmorValue",
        "C_CSPlayerPawnBase::m_vecVelocity",
        "C_CSPlayerPawnBase::m_bIsScoped",
        "C_CSPlayerPawnBase::m_aimPunchAngle",
        "C_PlantedC4::m_bBombTicking",
        "C_PlantedC4::m_flC4Blow",
        "C_PlantedC4::m_flDefuseCountDown",
        "C_PlantedC4::m_bBombDefused",
        "bone.head", "bone.neck", "bone.chest", "bone.pelvis",
    };

    auto active = app.offsets().active_cached();
    if (ImGui::BeginTable("##required", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch, 0.65f);
        ImGui::TableSetupColumn("status");
        ImGui::TableHeadersRow();
        for (auto name : required_fields) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(name);
            ImGui::TableNextColumn();
            if (active) {
                if (auto f = active->find(name)) {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImGui::ColorConvertU32ToFloat4(compat_color(f->compat)));
                    ImGui::Text("%s (0x%llX)", offsets::compat_label(f->compat),
                                (unsigned long long)f->value);
                    ImGui::PopStyleColor();
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImGui::ColorConvertU32ToFloat4(Palette::error));
                    ImGui::TextUnformatted("missing");
                    ImGui::PopStyleColor();
                }
            } else {
                ImGui::TextDisabled("no active bundle");
            }
        }
        ImGui::EndTable();
    }
}

void draw_source_manager(App& app) {
    ImGui::TextUnformatted("Source Manager");
    ImGui::TextDisabled("Public offset sources Vanta can query. Nothing is baked in.");

    auto sources = app.offsets().list_sources();
    const auto status = app.offsets().status();

    static char n_name[128] = "my-source";
    static char n_url[512]  = "";
    static char n_notes[256]= "";
    static int  n_timeout   = 5000;
    static int  n_retries   = 3;

    section_header("ADD / UPDATE SOURCE");
    ImGui::InputText("name", n_name, sizeof(n_name));
    ImGui::InputText("url (JSON over HTTPS)", n_url, sizeof(n_url));
    ImGui::InputText("notes",  n_notes, sizeof(n_notes));
    ImGui::InputInt("timeout (ms)", &n_timeout);
    ImGui::InputInt("retries",      &n_retries);
    if (ImGui::Button("Save source") && n_name[0] && n_url[0]) {
        offsets::SourceConfig c;
        c.name  = n_name;
        c.type  = "generic-json";
        c.url   = n_url;
        c.notes = n_notes;
        c.timeout_ms  = n_timeout;
        c.max_retries = n_retries;
        c.enabled     = true;
        app.offsets().upsert_source(c);
    }

    section_header("CATALOG");
    if (ImGui::BeginTable("##sources", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("name");
        ImGui::TableSetupColumn("url", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("state");
        ImGui::TableSetupColumn("last");
        ImGui::TableSetupColumn("actions");
        ImGui::TableHeadersRow();
        for (auto& s : sources) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(s.name.c_str());
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", s.url.c_str());

            const offsets::SourceStatus* st = nullptr;
            for (auto& x : status) if (x.name == s.name) { st = &x; break; }

            ImGui::TableNextColumn();
            if (!st) ImGui::TextDisabled("never fetched");
            else if (st->last_ok) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Palette::success));
                ImGui::TextUnformatted("ok");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Palette::error));
                ImGui::Text("err %s", st->last_error.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::TableNextColumn();
            if (st) ImGui::Text("%lldms", (long long)st->last_duration.count());
            else ImGui::TextDisabled("-");

            ImGui::TableNextColumn();
            ImGui::PushID(s.name.c_str());
            if (ImGui::SmallButton("Fetch")) {
                auto fut = app.offsets().fetch_async(s.name);
                // Fire-and-forget: the manager records status; auto-adopt when
                // the fetched bundle parses cleanly.
                std::thread([fut = std::move(fut), app = &app]() mutable {
                    auto r = fut.get();
                    if (r.ok && !r.bundle.fields.empty()) {
                        app->offsets().adopt(r.bundle);
                    }
                }).detach();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                app.offsets().remove_source(s.name);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void draw_version_history(App& app) {
    ImGui::TextUnformatted("Version History");
    auto hist = app.offsets().cache().history();
    if (hist.empty()) {
        ImGui::TextDisabled("No prior adoptions.");
        return;
    }
    if (ImGui::BeginTable("##hist", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("source");
        ImGui::TableSetupColumn("build");
        ImGui::TableSetupColumn("state");
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();
        for (std::size_t i = 0; i < hist.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%zu", i);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(hist[i].source_name.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(hist[i].game_build.c_str());
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text,
                ImGui::ColorConvertU32ToFloat4(compat_color(hist[i].overall())));
            ImGui::TextUnformatted(offsets::compat_label(hist[i].overall()));
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            ImGui::PushID((int)i);
            if (ImGui::SmallButton("Roll back to this")) {
                app.offsets().cache().rollback_to(i);
                app.offsets().refresh_active_from_disk();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace vanta::ui
