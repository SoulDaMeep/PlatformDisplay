#include "pch.h"

#include "PlatformDisplay.h"
#include "ScoreboardPositionInfo.h"

#include<functional>
#include <map>
#include <iostream>
#include <unordered_set>

namespace {

	BAKKESMOD_PLUGIN(PlatformDisplay, "Shows the platform of all the players in a game.", plugin_version, PLUGINTYPE_FREEPLAY)

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
	cvarManager->registerCvar("PlatformDisplay_AlphaConsole", "0", "Hides user's icon");
	cvarManager->registerCvar("PlatformDisplay_SteamPlayer", "0", "Show/hide icons for steam players");

	gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.GFxData_Scoreboard_TA.UpdateSortedPlayerIDs", [this](ActorWrapper caller, ...) {
		getSortedIds(caller);
		// Some gamemodes have different hooks (potentially dangerous)
		ComputeScoreboardInfo();
	});
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
		//close scorebaord at end of game instance
		scoreBoardOpen = false;
		
		comparisons.clear();
		disconnectedPris.clear();
		ComputeScoreboardInfo();
		// reset the list so "old data" doesnt leak into other gamemodes
	});

	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		RenderPlatformLogos(canvas);
	});
}

void PlatformDisplay::getSortedIds(ActorWrapper caller) {

	auto* scoreboard = reinterpret_cast<ScoreboardObj*>(caller.memory_address);
	if (scoreboard->sorted_names == nullptr) return;
	auto sorted_names = std::wstring(scoreboard->sorted_names);

	std::string str;
	// Turn wstring into string so we can later use string.find
	std::transform(sorted_names.begin(), sorted_names.end(), std::back_inserter(str), [](wchar_t c) {
		return (char)c;
		});
	sortedIds = str;
}

bool PlatformDisplay::sortPris(Pri a, Pri b) {
	std::string id_a = a.uid.GetIdString();
	std::string id_b = b.uid.GetIdString();

	// Bots have their id in sortedIds in the format of Bot_Name
	if (a.isBot) id_a = "Bot_" + a.name;
	if (b.isBot) id_b = "Bot_" + b.name;

	// Use the sorted ids string to find out which pri is higher on the scoreboard
	// Needed when the players all have a score of 0
	size_t index_a = sortedIds.find(id_a);
	size_t index_b = sortedIds.find(id_b);
	if (index_a != std::string::npos && index_b != std::string::npos) {
		return index_a < index_b;
	}
	// Fallback mechanism if sorted ids doesn't contain the ids
	else {
		return a.score > b.score;
	}
}

void PlatformDisplay::ComputeScoreboardInfo() {
	if (!accumulateComparisons) {
		return;
	}
	accumulateComparisons = false;

	auto hash = [](const Pri& p) { return std::hash<std::string>{}(nameAndId(p)); };
	auto keyEqual = [](const Pri& lhs, const Pri& rhs) { return nameAndId(lhs) == nameAndId(rhs); };
	std::unordered_set<Pri, decltype(hash), decltype(keyEqual)> seenPris{ 10, hash, keyEqual };
	// make sure it still shows when they rejoin
	disconnectedPris.clear();
	for (const auto& comparison : comparisons) {
		seenPris.insert(comparison.first);
		seenPris.insert(comparison.second);

		if (comparison.first.ghost_player) {
			disconnectedPris.insert(nameAndId(comparison.first));
		}
		if (comparison.second.ghost_player) {
			disconnectedPris.insert(nameAndId(comparison.second));
		}
	}

	std::vector<Pri> seenPrisVector;
	int numBlues{};
	int numOranges{};
	for (auto pri : seenPris) {
		pri.team = teamHistory[nameAndId(pri)];

		if (pri.team > 1) disconnectedPris.insert(nameAndId(pri));
		if(disconnectedPris.find(nameAndId(pri)) != disconnectedPris.end()) {
			pri.ghost_player = true;
		}

		if (pri.team == 0) {
			numBlues++;
		}
		else {
			numOranges++;
		}
		seenPrisVector.push_back(pri);
	}
	std::sort(seenPrisVector.begin(), seenPrisVector.end(), [this](const Pri& a, const Pri& b) { return sortPris(a, b); });
	computedInfo = ComputedScoreboardInfo{ seenPrisVector, numBlues, numOranges };
}

void PlatformDisplay::RecordScoreboardComparison(ActorWrapper gameEvent, void* params, std::string eventName) {
	if (!accumulateComparisons) {
		accumulateComparisons = true;
		comparisons.clear();
	}
	SSParams* p = static_cast<SSParams*>(params);
	if (!p) { LOG("NULL SSParams"); return; }
	PriWrapper a(p->PRI_A);
	PriWrapper b(p->PRI_B);
	comparisons.push_back({ a, b });
	auto teamNumA = a.GetTeamNum2();
	if (teamNumA <= 1) {
		teamHistory[nameAndId(a)] = teamNumA;
	}

	auto teamNumB = b.GetTeamNum2();
	if (teamNumB <= 1) {
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
	}
}

void PlatformDisplay::RenderPlatformLogos(CanvasWrapper canvas) {
	if (!scoreBoardOpen) { return; }
	if (!gameWrapper->IsInOnlineGame()) { return; }
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (!sw) { return; }
	if (sw.GetbMatchEnded()) { return; }

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
		gameWrapper->GetDisplayScale() * gameWrapper->GetInterfaceScale(),
		/* mutators= */ mmrWrapper.GetCurrentPlaylist() == 34,
		computedInfo.bluePlayerCount,
		computedInfo.orangePlayerCount);
	int blues = -1;
	int oranges = -1;

	for (auto pri : computedInfo.sortedPlayers) {
		Vector2F drawPos{};
		if (pri.team == 0) {
			blues++;
			canvas.SetColor(blueColor);
			if (pri.ghost_player) canvas.SetColor(LinearColor{ blueColor.R, blueColor.G, blueColor.B, 155 / 1.5 });
			drawPos = sbPosInfo.blueLeaderPos + Vector2F{ 0, sbPosInfo.playerSeparation * blues };
		}
		else if (pri.team == 1) {
			oranges++;
			canvas.SetColor(orangeColor);
			if (pri.ghost_player) canvas.SetColor(LinearColor{ orangeColor.R, orangeColor.G, orangeColor.B, 155 / 1.5 });
			drawPos = sbPosInfo.orangeLeaderPos + Vector2F{ 0, sbPosInfo.playerSeparation * oranges };
		}
		else {
			LOG("[PlatformDisplay] Unexpected team value {} for pri {}", pri.team, nameAndId(pri));
			continue;
		}
		if (pri.isBot) { continue; }
		CVarWrapper AlphaConsoleCvar = cvarManager->getCvar("PlatformDisplay_AlphaConsole");
		if(!AlphaConsoleCvar) {return;}
		bool AlphaConsole = AlphaConsoleCvar.getBoolValue();
		if (pri.platform == OnlinePlatform_Steam && !showIconsForSteamPlayers) { continue; }
		if (AlphaConsole && (pri.uid == gameWrapper->GetUniqueID())) {continue;}
		canvas.SetPosition(drawPos);
		std::shared_ptr<ImageWrapper> image = logoList[PlatformImageMap[pri.platform]];
		if (!image->IsLoadedForCanvas()) {
			LOG("[PlatformDisplay] Image not loaded for canvas.");
			continue;
		}
		canvas.DrawTexture(image.get(), 100.0f / 48.0f * sbPosInfo.profileScale); // last bit of scale b/c imgs are 48x48		
	}
}
