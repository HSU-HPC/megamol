/*
 * Call.cpp
 *
 * Copyright (C) 2019 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */


#include "Call.h"
#include "InterfaceSlot.h"
#include "Module.h"

#ifdef PROFILING
#define PROFILING_CHILD_WIDTH (18.0f)
#define PROFILING_CHILD_HEIGHT (14.8f)
#endif

using namespace megamol;
using namespace megamol::gui;


megamol::gui::Call::Call(ImGuiID uid, const std::string& class_name, const std::string& description,
    const std::string& plugin_name, const std::vector<std::string>& functions)
        : uid(uid)
        , class_name(class_name)
        , description(description)
        , plugin_name(plugin_name)
        , functions(functions)
        , connected_callslots()
        , gui_selected(false)
        , caller_slot_name()
        , callee_slot_name()
        , gui_tooltip()
#ifdef PROFILING
        , profiling()
        , show_profiling_data(false)
#endif // PROFILING
{

    this->connected_callslots.emplace(CallSlotType::CALLER, nullptr);
    this->connected_callslots.emplace(CallSlotType::CALLEE, nullptr);
}


megamol::gui::Call::~Call() {

    // Disconnect call slots
    this->DisconnectCallSlots();
}


bool megamol::gui::Call::IsConnected() {

    unsigned int connected = 0;
    for (auto& callslot_map : this->connected_callslots) {
        if (callslot_map.second != nullptr) {
            connected++;
        }
    }
    if (connected != 2) {
        /// megamol::core::utility::log::Log::DefaultLog.WriteWarn("[GUI] Call has only one connected call slot. [%s,
        /// %s, line %d]\n",
        /// __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    return (connected == 2);
}


bool megamol::gui::Call::ConnectCallSlots(
    megamol::gui::CallSlotPtr_t callslot_1, megamol::gui::CallSlotPtr_t callslot_2) {

    if ((callslot_1 == nullptr) || (callslot_2 == nullptr)) {
        megamol::core::utility::log::Log::DefaultLog.WriteWarn(
            "[GUI] Pointer to given call slot is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    if ((this->connected_callslots[callslot_1->Type()] != nullptr) ||
        (this->connected_callslots[callslot_2->Type()] != nullptr)) {
        megamol::core::utility::log::Log::DefaultLog.WriteWarn(
            "[GUI] Call is already connected. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    if (callslot_1->IsConnectionValid((*callslot_2))) {
        this->connected_callslots[callslot_1->Type()] = callslot_1;
        this->connected_callslots[callslot_2->Type()] = callslot_2;
        return true;
    }
    return false;
}


bool megamol::gui::Call::DisconnectCallSlots(ImGuiID calling_callslot_uid) {

    try {
        for (auto& callslot_map : this->connected_callslots) {
            if (callslot_map.second != nullptr) {
                if (callslot_map.second->UID() != calling_callslot_uid) {
                    callslot_map.second->DisconnectCall(this->uid);
                }
                callslot_map.second.reset();
            }
        }
    } catch (std::exception& e) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] Error: %s [%s, %s, line %d]\n", e.what(), __FILE__, __FUNCTION__, __LINE__);
        return false;
    } catch (...) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] Unknown Error. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    return true;
}


const megamol::gui::CallSlotPtr_t& megamol::gui::Call::CallSlotPtr(megamol::gui::CallSlotType type) {

    if (this->connected_callslots[type] == nullptr) {
        /// megamol::core::utility::log::Log::DefaultLog.WriteWarn("[GUI] Returned pointer to call slot is nullptr. [%s,
        /// %s, line %d]\n",
        /// __FILE__, __FUNCTION__, __LINE__);
    }
    return this->connected_callslots[type];
}


void megamol::gui::Call::Draw(megamol::gui::PresentPhase phase, megamol::gui::GraphItemsState_t& state) {

    if (ImGui::GetCurrentContext() == nullptr) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] No ImGui context available. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return;
    }

    ImGuiStyle& style = ImGui::GetStyle();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    assert(draw_list != nullptr);

    try {
        if (this->IsConnected()) {
            auto callerslot_ptr = this->CallSlotPtr(CallSlotType::CALLER);
            auto calleeslot_ptr = this->CallSlotPtr(CallSlotType::CALLEE);
            if ((callerslot_ptr == nullptr) || (calleeslot_ptr == nullptr)) {
                return;
            }
            this->caller_slot_name = callerslot_ptr->Name();
            this->callee_slot_name = calleeslot_ptr->Name();

            // Calls lie only completely inside or outside groups
            bool hidden = false;
            bool connect_interface_slot = true;
            if (callerslot_ptr->IsParentModuleConnected() && calleeslot_ptr->IsParentModuleConnected()) {
                if (callerslot_ptr->GetParentModule()->GroupUID() == calleeslot_ptr->GetParentModule()->GroupUID()) {
                    connect_interface_slot = false;
                    hidden = callerslot_ptr->GetParentModule()->IsHidden();
                }
            }
            if (!hidden) {

                ImVec2 caller_pos = callerslot_ptr->Position();
                ImVec2 callee_pos = calleeslot_ptr->Position();
                if (connect_interface_slot) {
                    if (callerslot_ptr->InterfaceSlotPtr() != nullptr) {
                        caller_pos = callerslot_ptr->InterfaceSlotPtr()->Position();
                    }
                    if (calleeslot_ptr->InterfaceSlotPtr() != nullptr) {
                        callee_pos = calleeslot_ptr->InterfaceSlotPtr()->Position();
                    }
                }

                ImGui::PushID(static_cast<int>(this->uid));

                /// COLOR_CALL_BACKGROUND
                ImVec4 tmpcol = style.Colors[ImGuiCol_FrameBg];
                tmpcol = ImVec4(tmpcol.x * tmpcol.w, tmpcol.y * tmpcol.w, tmpcol.z * tmpcol.w, 1.0f);
                const ImU32 COLOR_CALL_BACKGROUND = ImGui::ColorConvertFloat4ToU32(tmpcol);
                /// COLOR_CALL_HIGHTLIGHT
                tmpcol = style.Colors[ImGuiCol_FrameBgActive];
                tmpcol = ImVec4(tmpcol.x * tmpcol.w, tmpcol.y * tmpcol.w, tmpcol.z * tmpcol.w, 1.0f);
                const ImU32 COLOR_CALL_HIGHTLIGHT = ImGui::ColorConvertFloat4ToU32(tmpcol);
                /// COLOR_CALL_CURVE
                tmpcol = style.Colors[ImGuiCol_FrameBgHovered];
                tmpcol = ImVec4(tmpcol.x * tmpcol.w, tmpcol.y * tmpcol.w, tmpcol.z * tmpcol.w, 1.0f);
                const ImU32 COLOR_CALL_CURVE = ImGui::ColorConvertFloat4ToU32(tmpcol);
                /// COLOR_CALL_CURVE_HIGHLIGHT
                tmpcol = style.Colors[ImGuiCol_ButtonActive];
                tmpcol = ImVec4(tmpcol.x * tmpcol.w, tmpcol.y * tmpcol.w, tmpcol.z * tmpcol.w, 1.0f);
                const ImU32 COLOR_CALL_CURVE_HIGHLIGHT = ImGui::ColorConvertFloat4ToU32(tmpcol);
                /// COLOR_CALL_GROUP_BORDER
                tmpcol = style.Colors[ImGuiCol_ScrollbarGrabActive];
                tmpcol = ImVec4(tmpcol.x * tmpcol.w, tmpcol.y * tmpcol.w, tmpcol.z * tmpcol.w, 1.0f);
                const ImU32 COLOR_CALL_GROUP_BORDER = ImGui::ColorConvertFloat4ToU32(tmpcol);
                /// COLOR_TEXT
                const ImU32 COLOR_TEXT = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

                if (phase == megamol::gui::PresentPhase::RENDERING) {
                    bool hovered = (state.interact.button_hovered_uid == this->uid);

                    // Draw Curve
                    ImU32 color_curve = COLOR_CALL_CURVE;
                    if (hovered || this->gui_selected) {
                        color_curve = COLOR_CALL_CURVE_HIGHLIGHT;
                    }
                    /// Draw simple line if zooming is too small for nice bezier curves.
                    if (state.canvas.zooming < 0.25f) {
                        draw_list->AddLine(
                            caller_pos, callee_pos, color_curve, GUI_LINE_THICKNESS * state.canvas.zooming);
                    } else {
                        draw_list->AddBezierCubic(caller_pos,
                            caller_pos + ImVec2((50.0f * megamol::gui::gui_scaling.Get()), 0.0f),
                            callee_pos + ImVec2((-50.0f * megamol::gui::gui_scaling.Get()), 0.0f), callee_pos,
                            color_curve, GUI_LINE_THICKNESS * state.canvas.zooming);
                    }
                }

                if (state.interact.call_show_label || state.interact.call_show_slots_label) {
                    std::string slots_label = this->SlotsLabel();
                    auto slots_label_width = ImGui::CalcTextSize(slots_label.c_str()).x;
                    auto class_name_width = ImGui::CalcTextSize(this->class_name.c_str()).x;
                    ImVec2 call_center = ImVec2(caller_pos.x + (callee_pos.x - caller_pos.x) / 2.0f,
                        caller_pos.y + (callee_pos.y - caller_pos.y) / 2.0f);
                    auto call_name_width = 0.0f;
                    if (state.interact.call_show_label) {
                        call_name_width = std::max(call_name_width, class_name_width);
                    }
                    if (state.interact.call_show_slots_label) {
                        call_name_width = std::max(call_name_width, slots_label_width);
                    }
                    ImVec2 rect_size = ImVec2(call_name_width + (2.0f * style.ItemSpacing.x),
                        ImGui::GetFontSize() + (2.0f * style.ItemSpacing.y));
                    if (state.interact.call_show_label && state.interact.call_show_slots_label) {
                        rect_size.y += (ImGui::GetFontSize() + style.ItemSpacing.y);
                    }
#ifdef PROFILING
                    rect_size.x += ImGui::GetFrameHeightWithSpacing();
                    rect_size.y = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y;
#endif
                    ImVec2 call_rect_min =
                        ImVec2(call_center.x - (rect_size.x / 2.0f), call_center.y - (rect_size.y / 2.0f));
#ifdef PROFILING
                    if (this->show_profiling_data) {
                        rect_size =
                            ImVec2(((ImGui::GetFrameHeight() * PROFILING_CHILD_WIDTH) + style.ItemSpacing.x * 2.0f),
                                (ImGui::GetFrameHeight() * (PROFILING_CHILD_HEIGHT + 1.0f) + style.ItemSpacing.x));
                    }
#endif
                    ImVec2 call_rect_max = ImVec2((call_rect_min.x + rect_size.x), (call_rect_min.y + rect_size.y));

                    std::string button_label = "call_" + std::to_string(this->uid);

                    if (phase == megamol::gui::PresentPhase::INTERACTION) {

                        // Button
                        ImGui::SetCursorScreenPos(call_rect_min);
                        ImGui::SetItemAllowOverlap();
                        ImGui::InvisibleButton(button_label.c_str(), rect_size);
                        ImGui::SetItemAllowOverlap();
                        if (ImGui::IsItemActivated()) {
                            state.interact.button_active_uid = this->uid;
                        }
                        if (ImGui::IsItemHovered()) {
                            state.interact.button_hovered_uid = this->uid;
                        }

                        ImGui::PushFont(state.canvas.gui_font_ptr);

                        // Context Menu
                        if (ImGui::BeginPopupContextItem()) {
                            state.interact.button_active_uid = this->uid;

                            ImGui::TextDisabled("Call");
                            ImGui::Separator();

                            if (ImGui::MenuItem("Delete",
                                    state.hotkeys[HOTKEY_CONFIGURATOR_DELETE_GRAPH_ITEM].keycode.ToString().c_str())) {
                                state.interact.process_deletion = true;
                            }
                            ImGui::Separator();

                            ImGui::TextDisabled("Description");
                            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 13.0f);
                            ImGui::TextUnformatted(this->description.c_str());
                            ImGui::PopTextWrapPos();
                            ImGui::EndPopup();
                        }

                        // Hover Tooltip
                        if (!state.interact.call_show_slots_label) {
                            if (state.interact.call_hovered_uid == this->uid) {
                                this->gui_tooltip.ToolTip(slots_label, ImGui::GetID(button_label.c_str()), 0.5f, 5.0f);

                            } else {
                                this->gui_tooltip.Reset();
                            }
                        }

                        ImGui::PopFont();

                    } else if (phase == megamol::gui::PresentPhase::RENDERING) {

                        bool active = (state.interact.button_active_uid == this->uid);
                        bool hovered = (state.interact.button_hovered_uid == this->uid);
                        bool mouse_clicked_anywhere = ImGui::IsWindowHovered() && ImGui::GetIO().MouseClicked[0];

                        // Selection
                        if (!this->gui_selected && active) {
                            state.interact.call_selected_uid = this->uid;
                            this->gui_selected = true;
                            state.interact.callslot_selected_uid = GUI_INVALID_ID;
                            state.interact.modules_selected_uids.clear();
                            state.interact.group_selected_uid = GUI_INVALID_ID;
                            state.interact.interfaceslot_selected_uid = GUI_INVALID_ID;
                        }
                        // Deselection
                        else if (this->gui_selected && ((mouse_clicked_anywhere && !hovered) ||
                                                           (state.interact.call_selected_uid != this->uid))) {
                            this->gui_selected = false;
                            if (state.interact.call_selected_uid == this->uid) {
                                state.interact.call_selected_uid = GUI_INVALID_ID;
                            }
                        }

                        // Hovering
                        if (hovered) {
                            state.interact.call_hovered_uid = this->uid;
                        }
                        if (!hovered && (state.interact.call_hovered_uid == this->uid)) {
                            state.interact.call_hovered_uid = GUI_INVALID_ID;
                        }

                        // Draw Background
                        ImU32 call_bg_color =
                            (this->gui_selected || hovered) ? (COLOR_CALL_HIGHTLIGHT) : (COLOR_CALL_BACKGROUND);
                        draw_list->AddRectFilled(call_rect_min, call_rect_max, call_bg_color, GUI_RECT_CORNER_RADIUS);
                        draw_list->AddRect(
                            call_rect_min, call_rect_max, COLOR_CALL_GROUP_BORDER, GUI_RECT_CORNER_RADIUS);

                        // Draw Text
                        ImVec2 text_pos_left_upper =
                            (call_center + ImVec2(-(class_name_width / 2.0f), -0.5f * ImGui::GetFontSize()));
#ifdef PROFILING
                        // ImGui::PushFont(state.canvas.gui_font_ptr);
                        text_pos_left_upper = call_rect_min + style.ItemSpacing;
                        ImGui::SetCursorScreenPos(text_pos_left_upper);
                        if (ImGui::ArrowButton(
                                "###profiling", ((this->show_profiling_data) ? (ImGuiDir_Down) : (ImGuiDir_Up)))) {
                            this->show_profiling_data = !this->show_profiling_data;
                        }
                        this->gui_tooltip.ToolTip("Profiling");
                        if (this->show_profiling_data) {
                            text_pos_left_upper.y += ImGui::GetFrameHeight();
                            ImGui::SetCursorScreenPos(text_pos_left_upper);
                            this->draw_profiling_data();
                        }
                        text_pos_left_upper.x += ImGui::GetFrameHeightWithSpacing();
                        text_pos_left_upper.y += (ImGui::GetFrameHeight() - ImGui::GetFontSize()) * 0.5f;
                        if (this->show_profiling_data) {
                            text_pos_left_upper.y -= ImGui::GetFrameHeight();
                        }
                        // ImGui::PopFont();
#endif
                        if (state.interact.call_show_label && state.interact.call_show_slots_label) {
                            text_pos_left_upper.y -= (0.5f * ImGui::GetFontSize());
                        }
                        if (state.interact.call_show_label) {
                            draw_list->AddText(text_pos_left_upper,
                                ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]), this->class_name.c_str());
                        }
                        text_pos_left_upper =
                            (call_center + ImVec2(-(slots_label_width / 2.0f), -0.5f * ImGui::GetFontSize()));
                        if (state.interact.call_show_label && state.interact.call_show_slots_label) {
                            text_pos_left_upper.y += (0.5f * ImGui::GetFontSize());
                        }
                        if (state.interact.call_show_slots_label) {
                            // Caller
                            draw_list->AddText(text_pos_left_upper,
                                ImGui::ColorConvertFloat4ToU32(GUI_COLOR_SLOT_CALLER), this->caller_slot_name.c_str());
                            // Separator
                            text_pos_left_upper.x += ImGui::CalcTextSize(this->caller_slot_name.c_str()).x;
                            draw_list->AddText(text_pos_left_upper, COLOR_TEXT, this->slot_name_separator.c_str());
                            // Callee
                            text_pos_left_upper.x += ImGui::CalcTextSize(this->slot_name_separator.c_str()).x;
                            draw_list->AddText(text_pos_left_upper,
                                ImGui::ColorConvertFloat4ToU32(GUI_COLOR_SLOT_CALLEE), this->callee_slot_name.c_str());
                        }
                    }
                }

                ImGui::PopID();
            }
        }
    } catch (std::exception& e) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] Error: %s [%s, %s, line %d]\n", e.what(), __FILE__, __FUNCTION__, __LINE__);
        return;
    } catch (...) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] Unknown Error. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return;
    }
}

#ifdef PROFILING

void megamol::gui::Call::draw_profiling_data() {

    ImGui::BeginChild("call_profiling_info",
        ImVec2((ImGui ::GetFrameHeight() * PROFILING_CHILD_WIDTH), (ImGui::GetFrameHeight() * PROFILING_CHILD_HEIGHT)),
        false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoMove);

    ImGui::TextUnformatted("Profiling");
    ImGui::SameLine();
    ImGui::TextDisabled("[Callback Name]");
    ImGui::BeginTabBar("profiling", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll);
    auto func_cnt = this->profiling.size();
    for (size_t i = 0; i < func_cnt; i++) {
        auto& tab_label = this->profiling[i].name;
        if (ImGui::BeginTabItem(tab_label.c_str(), nullptr, ImGuiTabItemFlags_None)) {
            if (ImGui::BeginTable(("table_" + tab_label).c_str(), 2,
                    ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableColumnFlags_NoResize,
                    ImVec2(0.0f, 0.0f))) {
                ImGui::TableSetupColumn(("column_" + tab_label).c_str(), ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("LastCPUTime");
                ImGui::TableNextColumn();
                ImGui::Text("%.12f", this->profiling[i].lcput);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("AverageCPUTime");
                ImGui::TableNextColumn();
                ImGui::Text("%.12f", this->profiling[i].acput);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("NumCPUSamples");
                ImGui::TableNextColumn();
                ImGui::Text("%i", this->profiling[i].ncpus);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("CPU History");
                ImGui::TableNextColumn();
                ImGui::PlotLines("###cpuplot", this->profiling[i].hcpu.data(),
                    static_cast<int>(this->profiling[i].hcpu.size()), 0, nullptr, 0.0f, 16.0f,
                    ImVec2(core::PerformanceHistory::buffer_length * 2 * megamol::gui::gui_scaling.Get(),
                        (3.0f * ImGui::GetFrameHeight())));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("LastGPUTime");
                ImGui::TableNextColumn();
                ImGui::Text("%.12f", this->profiling[i].lgput);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("AverageGPUTime");
                ImGui::TableNextColumn();
                ImGui::Text("%.12f", this->profiling[i].agput);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("NumGPUSamples");
                ImGui::TableNextColumn();
                ImGui::Text("%.12i", this->profiling[i].ngpus);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("GPU History");
                ImGui::TableNextColumn();
                ImGui::PlotLines("###gpuplot", this->profiling[i].hgpu.data(),
                    static_cast<int>(this->profiling[i].hgpu.size()), 0, nullptr, 0.0f, 16.0f,
                    ImVec2(core::PerformanceHistory::buffer_length * 2 * megamol::gui::gui_scaling.Get(),
                        ((3.0f * ImGui::GetFrameHeight()))));
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();

    ImGui::EndChild();
}

#endif // PROFILING
