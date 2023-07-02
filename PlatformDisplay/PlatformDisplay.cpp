#include "pch.h"

#include "PlatformDisplay.h"
#include "ScoreboardPositionInfo.h"

#include<functional>
#include <map>
#include <iostream>
#include <unordered_set>

namespace {

BAKKESMOD_PLUGIN(PlatformDisplay, "Shows the platform of all the players in a game.", plugin_version, PLUGINTYPE_FREEPLAY)

const int BLUE_TEAM = 0;
const int ORANGE_TEAM = 1;

std::map<OnlinePlatform, int> PlatformImageMap{
	{ OnlinePlatform_Unknown,  0 },
	{ OnlinePlatform_Steam,  1 },
	{ OnlinePlatform_PS4, 2 },
	{ OnlinePlatform_PS3, 2},
	// Dingo = Xbox
	{ OnlinePlatform_Dingo, 3 },
	// NNX = Nintendo
	{ OnlinePlatform_OldNNX, 4},
	{ OnlinePlatform_NNX, 4},
	{ OnlinePlatform_Deleted, 0},
	{ OnlinePlatform_Epic, 5}
};

struct SSParams {
	uintptr_t PRI_A;
	uintptr_t PRI_B;

	// if hooking post
	int32_t ReturnValue;
};

std::string nameAndId(PriWrapper pri) {
	return pri.GetPlayerName().ToString() + "|" + pri.GetUniqueIdWrapper().GetIdString();
}

std::string nameAndId(const PlatformDisplay::Pri& p) {
	return p.name + "|" + p.uid.GetIdString();
}

class DebugViewConstants {
private:
	DebugViewConstants(float textHeight_, float boxWidth_, Vector2F iconTextOffset_) :
		textHeight(textHeight_), boxWidth(boxWidth_), iconTextOffset(iconTextOffset_) {}
public:
	static DebugViewConstants create(CanvasWrapper canvas, float textSize) {
		const float scalingFactor = canvas.GetSize().Y / 1440.0;
		return { 14 * textSize * scalingFactor,
				 500 * scalingFactor,
				 Vector2F{-200, 0} *scalingFactor };
	}
	const float textHeight{};
	const float boxWidth{};
	const Vector2F iconTextOffset{};
	const LinearColor backgroundColor{ 69, 69, 69, 169 };
	const LinearColor textColor{ 255, 255, 255, 255 };
};

} // namespace

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void PlatformDisplay::onLoad()
{
	_globalCvarManager = cvarManager;

	for (int i = 0; i < LOGO_COUNT; i++) {
		logos[i] = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / (std::to_string(i) + ".png"), false, false);
		logos[i]->LoadForCanvas();
	}
	for (int i = 0; i < LOGO_COUNT; i++) {
		noTintLogos[i] = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / (std::to_string(i) + "-no-tint.png"), false, false);
		noTintLogos[i]->LoadForCanvas();
	}

	cvarManager->registerCvar("PlatformDisplay_OverrideTints", "0", "Override the autotinting of the platform icons");
	cvarManager->registerCvar("PlatformDisplay_Enabled", "1", "Enable the plugin");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerBlueTeam", "#FFFFFF", "Changes the color of the text for Blue team");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerOrangeTeam", "#FFFFFF", "Changes the color of the text for Orange Team");
	cvarManager->registerCvar("PlatformDisplay_SteamPlayer", "0", "Show/hide icons for steam players");
	cvarManager->registerCvar("PlatformDisplay_DebugView", "0", "Show/hide debug view");

	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState", [this](std::string eventName) {
		SetTeamColors();
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.AddCar", [this](std::string eventName) {
		SetTeamColors();
	});
	gameWrapper->HookEventWithCaller<ActorWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.ScoreboardSort", 
		[this](ActorWrapper gameEvent, void* params, std::string eventName) {
		  RecordScoreboardComparison(gameEvent, params, eventName);
	});
	gameWrapper->HookEventWithCaller<ActorWrapper>(
		"Function TAGame.PRI_TA.GetScoreboardStats",
		[this](auto args...) { ComputeScoreboardInfo();	
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", [this](...) {
		scoreBoardOpen = true;
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", [this](...) {
		scoreBoardOpen = false;
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", [this](...) {
		comparisons.clear();
		ComputeScoreboardInfo();
	});

	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		RenderPlatformLogos(canvas);
	});
}

void PlatformDisplay::ComputeScoreboardInfo() {
	if (!accumulateComparisons) {
		return;
	}
	accumulateComparisons = false;

	auto hash = [](const Pri& p) { return std::hash<std::string>{}(nameAndId(p)); };
	auto keyEqual = [](const Pri& lhs, const Pri& rhs) { return nameAndId(lhs) == nameAndId(rhs); };
	std::unordered_set<Pri, decltype(hash), decltype(keyEqual)> seenPris{10, hash, keyEqual};
	for (const auto& comparison : comparisons) {
		seenPris.insert(comparison.first);
		seenPris.insert(comparison.second);
	}
	std::vector<Pri> seenPrisVector;
	int numBlues{};
	int numOranges{};
	for (auto pri : seenPris) {
		if (teamHistory[nameAndId(pri)] == BLUE_TEAM) {
			numBlues++;
		}
		else {
			numOranges++;
		}
		seenPrisVector.push_back(pri);
	}
	std::sort(seenPrisVector.begin(), seenPrisVector.end(), [](const Pri& a, const Pri& b) { return a.score > b.score; });
	computedInfo = ComputedScoreboardInfo{seenPrisVector, numBlues, numOranges};
}

void PlatformDisplay::RecordScoreboardComparison(ActorWrapper gameEvent, void* params, std::string eventName) {
	if (!accumulateComparisons) {
		accumulateComparisons = true;
		comparisons.clear();
	}
	SSParams* p = static_cast<SSParams*>(params);
	if (!p) { LOG("PlatformDisplay::RecordScorboardComparison: NULL SSParams"); return; }
	PriWrapper a(p->PRI_A);
	PriWrapper b(p->PRI_B);
	comparisons.push_back({ a, b });
	auto teamNumA = a.GetTeamNum2();
	if (teamNumA == BLUE_TEAM || teamNumA == ORANGE_TEAM) {
		teamHistory[nameAndId(a)] = teamNumA;
	}

	auto teamNumB = b.GetTeamNum2();
	if (teamNumB == BLUE_TEAM || teamNumB == ORANGE_TEAM) {
		teamHistory[nameAndId(b)] = teamNumB;
	}
}

void PlatformDisplay::SetTeamColors(bool keepOrder) {
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
	ServerWrapper sw = gameWrapper->IsInOnlineGame() ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer();
#else
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper sw = gameWrapper->GetOnlineGame();
#endif
	if (sw.IsNull()) return;

	ArrayWrapper<TeamWrapper> teams = sw.GetTeams();
	for (auto team : teams) {
		if (!team) {
			continue;
		}
		if (team.GetTeamNum2() < 0 || team.GetTeamNum2() > 1) {
			continue;
		}
		LinearColor unscaledColor = team.GetPrimaryColor();
		teamColors[team.GetTeamNum2()] = unscaledColor * 255.0f;
		LOG("Got {}, {}, {}, {} for {} color", teamColors[team.GetTeamNum2()].R, teamColors[team.GetTeamNum2()].G, teamColors[team.GetTeamNum2()].B, teamColors[team.GetTeamNum2()].A, team.GetTeamNum2());
	}
}

void PlatformDisplay::RenderPlatformLogos(CanvasWrapper canvas) {
	if (!gameWrapper->IsInOnlineGame()) { return; }
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (!sw) { return; }
	if (sw.GetbMatchEnded()) { return; }

	CVarWrapper debugCvar = cvarManager->getCvar("PlatformDisplay_DebugView");
	bool debugView = debugCvar && debugCvar.getBoolValue();
	if (debugView) {
		RenderDebugInfo(canvas);
	}

	if (!scoreBoardOpen) { return; }

	CVarWrapper enabledCvar = cvarManager->getCvar("PlatformDisplay_Enabled");
	bool enabled = enabledCvar ? enabledCvar.getBoolValue() : false;
	if (!enabled) { return; }
	
	CVarWrapper steamPlayerCvar = cvarManager->getCvar("PlatformDisplay_SteamPlayer");
	bool showIconsForSteamPlayers = steamPlayerCvar ? !steamPlayerCvar.getBoolValue() : true;

	LinearColor blueColor = teamColors[0];
	LinearColor orangeColor = teamColors[1];
	CVarWrapper overrideTintCvar = cvarManager->getCvar("PlatformDisplay_OverrideTints");
	int overrideTint = overrideTintCvar ? overrideTintCvar.getIntValue() : 0;
	if (overrideTint == 1) {
		blueColor = { 255, 255, 255, 155 };
		orangeColor = { 255, 255, 255, 155 };
	}
	else if (overrideTint == 2) {
		CVarWrapper blueColorPicker = cvarManager->getCvar("PlatformDisplay_ColorPickerBlueTeam");
		CVarWrapper orangeColorPicker = cvarManager->getCvar("PlatformDisplay_ColorPickerOrangeTeam");
		blueColor = blueColorPicker ? blueColorPicker.getColorValue() : blueColor;
		orangeColor = orangeColorPicker ? orangeColorPicker.getColorValue() : orangeColor;
	}
	auto logoList = overrideTint == 1 ? noTintLogos : logos;

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();	
	Vector2 screenSize = gameWrapper->GetScreenSize();
	Vector2F screenSizeFloat{ screenSize.X, screenSize.Y };
	SbPosInfo sbPosInfo = getSbPosInfo(screenSizeFloat,
		gameWrapper->GetDisplayScale(),
		/* mutators= */ mmrWrapper.GetCurrentPlaylist() == 34,
		computedInfo.bluePlayerCount,
		computedInfo.orangePlayerCount);

	int blues = -1;
	int oranges = -1;

	for (auto pri : computedInfo.sortedPlayers) {
		Vector2F drawPos{};
		int team = teamHistory[nameAndId(pri)];
		if (team == BLUE_TEAM) {
			blues++;
			canvas.SetColor(blueColor);
			drawPos = sbPosInfo.blueLeaderPos + Vector2F{ 0, sbPosInfo.playerSeparation * blues };
		}
		else if (team == ORANGE_TEAM) {
			oranges++;
			canvas.SetColor(orangeColor);
			drawPos = sbPosInfo.orangeLeaderPos + Vector2F{ 0, sbPosInfo.playerSeparation * oranges };
		}
		else {
			LOG("Unexpected team value {} for pri {}", pri.team, nameAndId(pri));
			continue;
		}
		canvas.SetPosition(drawPos);
		std::shared_ptr<ImageWrapper> image = logoList[PlatformImageMap[pri.platform]];
		if (!image->IsLoadedForCanvas()) {
			LOG("Image not loaded for canvas.");
			continue;
		}
		if (!pri.isBot && (showIconsForSteamPlayers || pri.platform != OnlinePlatform_Steam)) {
			canvas.DrawTexture(image.get(), 100.0f / 48.0f * sbPosInfo.profileScale); // last bit of scale b/c imgs are 48x48
		}
		if (debugView) {
			RenderDebugPri(canvas, pri, drawPos);
		}
	}
}

void PlatformDisplay::RenderDebugInfo(CanvasWrapper canvas) {
	float textSize = 2;
	auto dvc = DebugViewConstants::create(canvas, textSize);

	// Draw comparisons.
	float comparisonX = canvas.GetSize().X - dvc.boxWidth;

	Vector2F drawPos{ comparisonX, 0 };
	Vector2F lineVec {0, dvc.textHeight};

	canvas.SetPosition(drawPos);
	canvas.SetColor(dvc.backgroundColor);
	canvas.FillBox(Vector2F{ dvc.boxWidth, dvc.textHeight * (comparisons.size() + 2.0f) });

	canvas.SetPosition(drawPos);
	canvas.SetColor(dvc.textColor);
	canvas.DrawString("Sort comparisons (" + std::to_string(comparisons.size()) + ")",
		textSize,
		textSize);

	drawPos += lineVec * 1.5;
	for (const auto& comparison : comparisons) {
		canvas.SetPosition(drawPos);
		std::string a = comparison.first.name;
		a.resize(12, ' ');
		std::string b = comparison.second.name;
		b.resize(12, ' ');
		std::string line = a + " _?_ " + b;
		canvas.DrawString(line, textSize, textSize);
		drawPos += lineVec;
	}

	// Draw sorted vector.
	drawPos += lineVec * 3;
	canvas.SetPosition(drawPos);
	canvas.SetColor(dvc.backgroundColor);
	canvas.FillBox(Vector2F{ dvc.boxWidth, dvc.textHeight * (computedInfo.sortedPlayers.size() + 2) });

	canvas.SetColor(dvc.textColor);
	canvas.SetPosition(drawPos);
	canvas.DrawString("Sorted player list (" + std::to_string(computedInfo.sortedPlayers.size()) + ")",
		textSize,
		textSize);

	drawPos += lineVec / 2;

	for (const auto& player : computedInfo.sortedPlayers) {
		drawPos += lineVec;
		canvas.SetPosition(drawPos);
		canvas.DrawString(player.name, textSize, textSize);
	}
}

void PlatformDisplay::RenderDebugPri(CanvasWrapper canvas, const PlatformDisplay::Pri& pri, Vector2F iconPos) {

	float textSize = 1;
	auto dvc = DebugViewConstants::create(canvas, textSize);
	Vector2F textPos = iconPos + dvc.iconTextOffset;

	canvas.SetColor(dvc.textColor);
	canvas.SetPosition(textPos);

	auto drawLine = [&canvas, textSize, textHeight = dvc.textHeight, &textPos](std::string s) {
		canvas.DrawString(s, textSize, textSize);
		textPos += Vector2F{ 0, textHeight };
		canvas.SetPosition(textPos);
	};
	drawLine("Name:  " + pri.name);
	drawLine("UID:   " + pri.uid.GetIdString());
	drawLine("Team:  " + std::to_string(pri.team));
	drawLine("Score: " + std::to_string(pri.score));
}
