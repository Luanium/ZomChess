#ifndef UI_H
#define UI_H

#include <imgui.h>

inline void SetupTacticalImGuiTheme() {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.35f;

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 6.0f;     
    style.WindowRounding = 8.0f;    
    style.FramePadding = ImVec2(10, 8); 

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.38f, 0.28f, 1.0f);        
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.52f, 0.38f, 1.0f); 
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.30f, 0.22f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.25f, 0.28f, 1.0f);
}

#endif // UI_H
