#pragma once
#include <nlohmann/json.hpp>

struct Settings {
	bool Enabled;
	bool SteamPlayer;
	bool AlphaConsole;
	int selected;
	
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, Enabled, SteamPlayer, AlphaConsole, selected)
};
