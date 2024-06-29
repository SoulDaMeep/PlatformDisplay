#include "pch.h"
#include "PlatformDisplay.h"
#include "IMGUI/imgui_internal.h"


std::string PlatformDisplay::GetPluginName() {
	return "PlatformDisplay";
}

void PlatformDisplay::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Render the plugin settings here
// This will show up in bakkesmod when the plugin is loaded at
//  f2 -> plugins -> PlatformDisplay
void PlatformDisplay::RenderSettings() {

	ImGuiStyle& style = ImGui::GetStyle();
	UserStyle = style;

	style.FrameRounding = 1.5f;
	style.FramePadding = ImVec2{ 2.0f, 5.0f };
	style.FrameBorderSize = 0.5f;
	
	style.Colors[ImGuiCol_Button] = ImVec4{ 0.0f, 193.0f / 255.0f, 173.0f / 255.0f, 1.0f };
	style.Colors[ImGuiCol_CheckMark] = ImVec4{ 0.0f / 255.0f, 198.0f / 255.0f, 173.0f / 255.0f, 1.0f };

	style.Colors[ImGuiCol_Border] = ImVec4{ 1.0f, 1.0f, 1.0f, 0.2f };
	style.Colors[ImGuiCol_BorderShadow] = ImVec4{ 1.0f, 1.0f, 1.0f, 0.0f };


	style.Colors[ImGuiCol_FrameBg] = ImVec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f };

	auto pTex = UserIcon->GetImGuiTex();
	if (UserIcon->IsLoadedForImGui() && pTex) {
		ImGui::Indent(5.0f);
		ImGui::Text("Display Mode");

		ImGui::Indent(15.0f);

		ImDrawList* drawList = ImGui::GetWindowDrawList();


		static float size = 65.0f;
		static float margin = 5.0f;
		static float element_size = 7.5f;
		static float gap = 5.0f;
		static float rounding = 8.0f;
		static float image_size = 40.0f;

		ImVec2 pos = ImGui::GetCursorScreenPos();
		for (int i = 0; i < 3; i++) {

			ImRect s(pos + ImVec2{size * i + (gap*i), 0}, ImVec2{pos.x + (size*i + (gap*i)) + size , pos.y + size});
			bool sHovered = false;
			bool sHeld = false;

			ImGuiID id = ImGui::GetID("##shape" + i);
			ImGui::ItemAdd(s, id);


			bool sPressed = ImGui::ButtonBehavior(s, id, &sHovered, &sHeld);
			
			ImVec4 innerColor = ImVec4{ 41.0f / 255.0f, 41.0f / 255.0f, 41.0f / 255.0f, 1.0f };
			ImVec4 displayColor = ImVec4{ 0.0f / 255.0f, 198.0f / 255.0f, 173.0f / 255.0f, 1.0f };
			if (sHovered) {
				innerColor = ImVec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f };
				displayColor = ImVec4{ 0.0f / 255.0f, 198.0f / 255.0f, 173.0f / 255.0f, 0.8f };
				if (sHeld) {
					innerColor = ImVec4{ 21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f };
					displayColor = ImVec4{ 0.0f / 255.0f, 198.0f / 255.0f, 173.0f / 255.0f, 0.5f };
				}
			}
			if (sPressed) {

				settings.selected = i;
				WriteSettings();
			}
			if (i < 2) {
				drawList->AddRectFilled(s.Min, s.Max, ImGui::ColorConvertFloat4ToU32(innerColor), 6.0f, ImDrawCornerFlags_All);

				ImVec2 cornerPos = ImVec2{ (s.Min.x + (size-(element_size*2) - margin)), s.Min.y + margin};

				float as = size - (margin * 2);
				float thing = margin + (as - image_size) / 2.0f;

				if (i == 1) rounding = 10.0f;
				else rounding = 4.0f;

				drawList->AddRectFilled(cornerPos, ImVec2{cornerPos.x + element_size*2, cornerPos.y + element_size*2}, ImGui::ColorConvertFloat4ToU32(displayColor), rounding, ImDrawCornerFlags_All);

				ImGui::SetCursorScreenPos(ImVec2{ (s.Min.x + thing) , s.Min.y + (margin * 2) + (element_size) });
				ImGui::Image(pTex, ImVec2{ image_size, image_size }, ImVec2{0, 0}, ImVec2{1, 1}, ImVec4{1.0f, 1.0f, 1.0f, 41.0f/255.0f});
			}
			else {
				drawList->AddRectFilled(s.Min, s.Max, ImGui::ColorConvertFloat4ToU32(displayColor), 8.0f, ImDrawCornerFlags_All);
			}
			if (settings.selected == i) {
				drawList->AddRect(      s.Min, s.Max, ImGui::ColorConvertFloat4ToU32(ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f }), 8.0f, ImDrawCornerFlags_All, 1.0f);
			}
		}
	}

	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2{ 0.0f, 15.0f });
	ImGui::Spacing();
	ImGui::Indent(-15.0f);
	ImGui::Text("Settings");
	ImGui::Spacing();
	ImGui::Indent(15.0f);
	if (ImGui::Checkbox("Enable plugin", &settings.Enabled)) WriteSettings();

	if (ImGui::Checkbox("Hide Steam", &settings.SteamPlayer)) WriteSettings();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly)) {
		ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
		ImGui::Begin("tool", &windowOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("If you are a steam player with a pfp, you can turn on\nthis so you can see other steam players pfp without\nthe icons getting in the way");
		ImGui::End();
	}
	if (ImGui::Checkbox("Hide Own", &settings.AlphaConsole)) WriteSettings();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly)) {
		ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
		ImGui::Begin("tool", &windowOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Hides your icon because...well you already know yours...\nHelpfull for people who use the Avatar feature in ACPlugin.");
		ImGui::End();
	}

	style = UserStyle;
}

// Do ImGui rendering here
void PlatformDisplay::Render()
{
	if (!ImGui::Begin(menuTitle.c_str(), &windowOpen, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!windowOpen)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string PlatformDisplay::GetMenuName()
{
	return "PlatformDisplay";
}

// Title to give the menu
std::string PlatformDisplay::GetMenuTitle()
{
	return menuTitle;
}
// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool PlatformDisplay::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool PlatformDisplay::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void PlatformDisplay::OnOpen()
{
	windowOpen = true;
}

// Called when window is closed
void PlatformDisplay::OnClose()
{
	windowOpen = false;
}