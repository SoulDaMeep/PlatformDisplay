#pragma once
#include <nlohmann/json.hpp>

struct Settings {
	bool Enabled;

	bool CustomTint;
	int32_t ARGBTintBlue;
	int32_t ARGBTintOrange;

	bool SteamPlayer;
	bool AlphaConsole;

	int selected;
	bool Offset;
	float offsetX;
	float offsetY;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, Enabled, CustomTint, ARGBTintBlue, ARGBTintOrange, SteamPlayer, AlphaConsole, selected, Offset, offsetX, offsetY)
};
