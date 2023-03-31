#include "pch.h"
#include "PlatformDisplay.h"


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
	ImGui::TextUnformatted("PlatformDisplay plugin settings");
	//blue color picker
	CVarWrapper colorpickerblue = cvarManager->getCvar("PlatformDisplay_ColorPickerBlueTeam");
	if (!colorpickerblue) { return; }
	LinearColor textColorBlue = colorpickerblue.getColorValue() / 255;
	//orange color picker
	CVarWrapper colorpickerorange = cvarManager->getCvar("PlatformDisplay_ColorPickerOrangeTeam");
	if (!colorpickerorange) { return; }
	LinearColor textColorOrange = colorpickerorange.getColorValue() / 255;
	//enabled
	CVarWrapper enabledCvar = cvarManager->getCvar("PlatformDisplay_Enabled");
	if (!enabledCvar) { return; }
	bool enabled = enabledCvar.getBoolValue();

	CVarWrapper steamPlayerCvar = cvarManager->getCvar("PlatformDisplay_SteamPlayer");
	if(!steamPlayerCvar) { return; }
	bool steamPlayer = steamPlayerCvar.getBoolValue();
	if (ImGui::Checkbox("Enable plugin", &enabled)) {
		enabledCvar.setValue(enabled);
	}

	CVarWrapper overrideTintCvar = cvarManager->getCvar("PlatformDisplay_OverrideTints");
	if (!overrideTintCvar) { return; }
	int doOverride = overrideTintCvar.getIntValue();

	static int overrideMode = 0;
	ImGui::RadioButton("Automatically tint icons", &overrideMode, 0); ImGui::SameLine();
	ImGui::RadioButton("Don't tint icons", &overrideMode, 1); ImGui::SameLine();
	ImGui::RadioButton("Override icon tint", &overrideMode, 2);
	if (ImGui::Checkbox("Steam Player", &steamPlayer)) {
		steamPlayerCvar.setValue(steamPlayer);
	}
	overrideTintCvar.setValue(overrideMode);

	if (overrideMode == 2) {
		if (ImGui::ColorEdit4("Blue Color", &textColorBlue.R)) {
			colorpickerblue.setValue(textColorBlue * 255);
		}
		if (ImGui::ColorEdit4("Orange Color", &textColorOrange.R)) {
			colorpickerorange.setValue(textColorOrange * 255);
		}
	}
}



// Do ImGui rendering here
void PlatformDisplay::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!isWindowOpen_)
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
	return menuTitle_;
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
	isWindowOpen_ = true;
}

// Called when window is closed
void PlatformDisplay::OnClose()
{
	isWindowOpen_ = false;
}

