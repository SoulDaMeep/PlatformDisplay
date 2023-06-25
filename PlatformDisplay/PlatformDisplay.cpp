#include "pch.h"

#include "PlatformDisplay.h"
#include "ScoreboardPositionInfo.h"

#include <map>
#include <iostream>

namespace {

BAKKESMOD_PLUGIN(PlatformDisplay, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)

std::map<int, std::string> PlatformMap{
	{ 0,  "[Unknown]" },
	{ 1,  "[Steam]"   },
	{ 2,  "[PS4]"     },
	{ 3,  "[PS4]"     },
	{ 4,  "[XboxOne]" },
	{ 6,  "[Switch]"  },
	{ 7,  "[Switch]"  },
	{ 9,  "[Deleted]" },
	{ 11, "[Epic]"    },
};

std::map<int, int> PlatformImageMap{
	{ 0,  0 },
	{ 1,  1 },
	{ 2,  2 },
	{ 3,  2 },
	{ 4,  3 },
	{ 6,  4 },
	{ 7,  4 },
	{ 9,  0 },
	{ 11, 5 }
};

float scale = 0.65f;

struct SSParams {
	uintptr_t PRI_A;
	uintptr_t PRI_B;

	// if hooking post
	int32_t ReturnValue;
};

std::string nameAndId(PriWrapper pri) {
	return pri.GetPlayerName().ToString() + "|" + pri.GetUniqueIdWrapper().GetIdString();
}

std::string nameAndId(PlatformDisplay::pri p) {
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
		notintlogos[i] = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / (std::to_string(i) + "-no-tint.png"), false, false);
		notintlogos[i]->LoadForCanvas();
	}

	cvarManager->registerCvar("PlatformDisplay_OverrideTints", "0", "Override the autotinting of the platform icons");
	cvarManager->registerCvar("PlatformDisplay_Enabled", "1", "Enable the plugin");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerBlueTeam", "#FFFFFF", "Changes the color of the text for blue team");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerOrangeTeam", "#FFFFFF", "Changes the color of the text for Orange Team");
	cvarManager->registerCvar("PlatformDisplay_SteamPlayer", "0", "If you use steam, check this");

	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState", [this](std::string eventName) {
		SetTeamColors();
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.AddCar", [this](std::string eventName) {
		SetTeamColors();
	});
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_Soccar_TA.ScoreboardSort", [this](ActorWrapper gameEvent, void* params, std::string eventName) {
		RecordScoreboardComparison(gameEvent, params, eventName);
	});
	gameWrapper->HookEventWithCaller<ActorWrapper>(
		"Function TAGame.PRI_TA.GetScoreboardStats",
		[this](auto args...) { ComputeScoreboardInfo();	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", [this](...) {
		scoreBoardOpen = true;
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", [this](...) {
		scoreBoardOpen = false;
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", [this](...) {
		comparedPris.clear();
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

	if (a.GetTeamNum2() > 1 && comparedPris.find(nameAndId(a)) != comparedPris.end()) {
		comparedPris[nameAndId(a)].score = a.GetMatchScore();
	}
	else {
		comparedPris[nameAndId(a)] = a;
	}
	if (b.GetTeamNum2() > 1 && comparedPris.find(nameAndId(b)) != comparedPris.end()) {
		comparedPris[nameAndId(b)].score = b.GetMatchScore();
	}
	else {
		comparedPris[nameAndId(b)] = b;
	}

	LOG("{} _ {}", a.GetPlayerName().ToString(), b.GetPlayerName().ToString());
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
	CVarWrapper enabledCvar = cvarManager->getCvar("PlatformDisplay_Enabled");
	if (!enabledCvar) { return; }
	bool enabled = enabledCvar.getBoolValue();
	if (scoreBoardOpen && enabled) {
		CVarWrapper BlueColorPicker = cvarManager->getCvar("PlatformDisplay_ColorPickerBlueTeam");
		CVarWrapper overrideTintCvar = cvarManager->getCvar("PlatformDisplay_OverrideTints");
		CVarWrapper OrangeColorPicker = cvarManager->getCvar("PlatformDisplay_ColorPickerOrangeTeam");
		CVarWrapper steamPlayerCvar = cvarManager->getCvar("PlatformDisplay_SteamPlayer");
		LinearColor BlueColor = BlueColorPicker.getColorValue();
		LinearColor OrangeColor = OrangeColorPicker.getColorValue();
		if (!BlueColorPicker) { return; }
		if (!overrideTintCvar) { return; }
		if (!OrangeColorPicker) { return; }
		if (!steamPlayerCvar) { return; }
		// the values
		
		if (!gameWrapper->IsInOnlineGame()) return;
		ServerWrapper sw = gameWrapper->GetOnlineGame();
		if (sw.IsNull()) return;

		int doOverride = overrideTintCvar.getIntValue();

		MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
		if (sw.GetbMatchEnded()) return;

		// prune
		auto server = gameWrapper->GetCurrentGameState();
		if (!server) { return; }
		auto activePris = server.GetPRIs();
		auto copy = comparedPris;
		for (auto [key, value] : copy) {
			// 2 cases
			// bot
			// zero points
			if (value.isBot || value.score == 0) {
				bool shouldPrune = true;
				for (int i = 0; i < activePris.Count(); i++) {
					PriWrapper pri = activePris.Get(i);
					if (!pri) { continue; }
					if (nameAndId(value) == nameAndId(pri)) {
						shouldPrune = false;
					}
				}
				if (shouldPrune) {
					comparedPris.erase(key);
				}
			}
		}

		std::vector<pri> pris;
		for (auto [key, value] : comparedPris) {
			pris.push_back(value);
		}
		std::sort(pris.begin(), pris.end(), [](pri a, pri b) {return a.score > b.score; });
		int num_blues{}, num_oranges{};
		for (auto pri: pris) {
			if (pri.team == 0) {
				num_blues++;
			}
			else if (pri.team == 1) {
				num_oranges++;
			}
		}
		Vector2 screenSize = gameWrapper->GetScreenSize();
		Vector2F screenSizeFloat = { screenSize.X, screenSize.Y };
		SbPosInfo sbPosInfo = getSbPosInfo(screenSizeFloat,
			gameWrapper->GetDisplayScale(),
			/* mutators= */ mmrWrapper.GetCurrentPlaylist() == 34,
			num_blues,
			num_oranges);

		int blues = -1;
		int oranges = -1;

		for (auto pri : pris) {
		//for (auto pri: comparedPris) {
		//for (auto pri : leaderboard) {
			if (pri.team == 0) {
				blues++;
			}
			else if (pri.team == 1) {
				oranges++;
			}
			if (pri.isBot || pri.team > 1) {
				continue;
			}

			std::string playerName = pri.name; // "playername"
			std::string playerOS = PlatformMap.at(pri.platform); // "os"
			std::string playerString = playerOS + playerName; // "[OS]playername"

			Vector2F drawPos;
			if (pri.team == 0) {
				drawPos = sbPosInfo.blueLeaderPos + Vector2F{0, sbPosInfo.playerSeparation * blues };
			}
			else {
				drawPos = sbPosInfo.orangeLeaderPos + Vector2F{0, sbPosInfo.playerSeparation * oranges };
			}

			canvas.SetPosition(drawPos);
			//canvas.FillBox(Vector2{(int)(100.0*image_scale),(int)( 100.0*image_scale)});

			// 2x scale
			//canvas.DrawString(playerString, 2.0, 2.0);
			int platformImage = pri.platform;
			std::shared_ptr<ImageWrapper> image = (doOverride == 1)? notintlogos[PlatformImageMap[platformImage]] : logos[PlatformImageMap[platformImage]];
			if (image->IsLoadedForCanvas()) {
				if (doOverride == 1) {
					canvas.SetColor({ 255, 255, 255, 155 });
				}
				else if (doOverride == 2) {
					if (pri.team == 0) {
						canvas.SetColor(BlueColorPicker.getColorValue());
						LOG("Using color {}", BlueColorPicker.getStringValue());
					}
					else {
						canvas.SetColor(OrangeColorPicker.getColorValue());
						LOG("Using color {}", OrangeColorPicker.getStringValue());
					}
				}
				else {
					canvas.SetColor(teamColors[pri.team]);
				}
					// if steam player, dont show steam 
					// if not steam player, show everything
					//if not a steam player
					if(!steamPlayerCvar.getBoolValue()) {
						canvas.DrawTexture(image.get(), 100.0f / 48.0f * sbPosInfo.profileScale); // last bit of scale b/c imgs are 48x48
						//if steam player
					} else if(steamPlayerCvar.getBoolValue() && pri.platform!=1) {
						canvas.DrawTexture(image.get(), 100.0f / 48.0f * sbPosInfo.profileScale); // last bit of scale b/c imgs are 48x48
					}
			}
			else {
				LOG("not loaded for canvas");
			}
		}
	}
}
