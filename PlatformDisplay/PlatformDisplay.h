#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class PlatformDisplay :
	public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow, public BakkesMod::Plugin::PluginWindow
	{

	std::shared_ptr<bool> enabled;

	virtual void onLoad();
	virtual void onUnload();

	void RenderSettings() override;
	std::string GetPluginName() override;

	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "PlatformDisplay";

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	void getNamesAndPlatforms();
	void Render(CanvasWrapper canvas);

	struct image {
		std::shared_ptr<ImageWrapper> img;
		Vector2 position;
		float scale;
	};
	struct pri {
		UniqueIDWrapper uid;
		int score;
		unsigned char team;
		bool isBot;
		std::string name;
		OnlinePlatform platform;
	};

	LinearColor teamColors[2];

	std::vector<pri> leaderboard;
	std::unordered_map<std::string, int> mmrs;
	int num_blues = 0;
	int num_oranges = 0;

	std::vector<image> toRender;
	void updateScores(bool keepOrder = false);
	bool compareName(int mmr1, std::string name1, int mmr2, std::string name2);
	std::string to_lower(std::string s);
	bool isSBOpen = false;
	bool isReplaying = false;

	std::shared_ptr<ImageWrapper> logos[6];
	std::shared_ptr<ImageWrapper> notintlogos[6];
};

