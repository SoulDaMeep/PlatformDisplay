#include "pch.h"
#include "PlatformDisplay.h"
#include <map>
#include <iostream>

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

// for both teams
// { {"Player1Name", "OS"}, {"Player2Name", "OS"}, {"Player3Name", "OS"}, }
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
std::vector<std::vector<std::string>> BlueTeamValues;
std::vector<std::vector<std::string>> OrangeTeamValues;
std::vector<int> blueTeamLogos;
std::vector<int> orangeTeamLogos;

float Gap = 0.0f;
bool show = false;
std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
void PlatformDisplay::onLoad()
{
	_globalCvarManager = cvarManager;
	for (int i = 0; i < 6; i++) {
		logos[i] = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / (std::to_string(i) + ".png"), false, false);
		logos[i]->LoadForCanvas();
	}
	for (int i = 0; i < 6; i++) {
		notintlogos[i] = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / (std::to_string(i) + "-no-tint.png"), false, false);
		notintlogos[i]->LoadForCanvas();
	}

	//Sounds like a lot of HOOPLA!!!
	cvarManager->registerCvar("PlatformDisplay_OverrideTints", "0", "Override the autotinting of the platform icons");
	cvarManager->registerCvar("PlatformDisplay_Enabled", "1", "Enable the plugin");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerBlueTeam", "#FFFFFF", "Changes the color of the text for blue team");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerOrangeTeam", "#FFFFFF", "Changes the color of the text for Orange Team");
	cvarManager->registerCvar("PlatformDisplay_SteamPlayer", "0", "If you use steam, check this");
	//Make thing go yes
	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		Render(canvas);
	});

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", [this](std::string eventName) {
		BlueTeamValues.clear();
		OrangeTeamValues.clear();
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", [this](std::string eventName) {
		show = true;
		updateScores();
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", [this](std::string eventName) {
		show = false;
	});

	//std::bind instead
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState", [this](std::string eventName) {
		updateScores();
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.AddCar", [this](std::string eventName) {
		leaderboard.clear();
		updateScores();
	});
	//
}

void PlatformDisplay::updateScores(bool keepOrder) {
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
	ServerWrapper sw = gameWrapper->IsInOnlineGame() ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer();
#else
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper sw = gameWrapper->GetOnlineGame();
#endif

	if (sw.IsNull()) return;
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (mmrWrapper.GetCurrentPlaylist() == 6) return;

	ArrayWrapper<TeamWrapper> teams = sw.GetTeams();
	for (auto team : teams) {
		if (!team) {
			continue;
		}
		if (team.GetTeamNum2() < 0 || team.GetTeamNum2() > 1) {
			continue;
		}
		LinearColor unscaledColor = team.GetPrimaryColor();
		teamColors[team.GetTeamNum2()] = { unscaledColor.R * 255, unscaledColor.G * 255, unscaledColor.B * 255, unscaledColor.A * 255};
		LOG("Got {}, {}, {}, {} for {} color", teamColors[team.GetTeamNum2()].R, teamColors[team.GetTeamNum2()].G, teamColors[team.GetTeamNum2()].B, teamColors[team.GetTeamNum2()].A, team.GetTeamNum2());
	}

	int currentPlaylist = mmrWrapper.GetCurrentPlaylist();
	std::vector<std::string> currentUids;
	std::vector<std::string> currentNames;
	bool isThereNew = false;
	int blues = 0;
	int oranges = 0;
	ArrayWrapper<PriWrapper> players = sw.GetPRIs();

	for (size_t i = 0; i < players.Count(); i++)
	{
		PriWrapper priw = players.Get(i);
		if (priw.IsNull()) continue;

		UniqueIDWrapper uidw = priw.GetUniqueIdWrapper();
		int score = priw.GetMatchScore();
		int size = leaderboard.size();
		int index = size;
		int toRemove = -1;
		bool isNew = true;
		bool isBot = priw.GetbBot();

		UniqueIDWrapper uniqueID = priw.GetUniqueIdWrapper(); //here
		OnlinePlatform platform = uniqueID.GetPlatform(); //here

		int mmr = 0;
		if (mmrs.find(uidw.GetIdString()) != mmrs.end()) { mmr = mmrs[uidw.GetIdString()]; }
		else {
			if (mmrWrapper.IsSynced(uidw, currentPlaylist) && !isBot) mmr = mmrWrapper.GetPlayerMMR(uidw, currentPlaylist);
			mmrs[uidw.GetIdString()] = mmr;
		}

		std::string name = priw.GetPlayerName().ToString(); currentNames.push_back(name);
		currentUids.push_back(uidw.GetIdString());

		for (size_t j = 0; j < size; j++)
		{
			if (index == size) {
				if (leaderboard[j].score < score) {
					index = j;
				}
				else if (leaderboard[j].score == score) {
					if (compareName(mmr, name, mmrs[leaderboard[j].uid.GetIdString()], leaderboard[j].name) && !isBot) index = j;
				}
			}
			if (uidw.GetIdString() == leaderboard[j].uid.GetIdString() && name == leaderboard[j].name) {
				toRemove = j;
				isNew = false;
			}
		}

		if (isNew) { isThereNew = true; getNamesAndPlatforms(); }
		if (toRemove > -1 && !keepOrder) {
			leaderboard.erase(leaderboard.begin() + toRemove);
			if (index > toRemove) index--;
		}
		if (isNew || !keepOrder) {
			leaderboard.insert(leaderboard.begin() + index, { uidw, score, priw.GetTeamNum(), isBot, name, platform });
		}
		else {
			leaderboard[toRemove].team = priw.GetTeamNum();
		}
		if (priw.GetTeamNum() == 0) blues++;
		else if (priw.GetTeamNum() == 1) oranges++;
	}
	num_blues = blues;
	num_oranges = oranges;

	//remove pris, that no longer exist
	if (isThereNew || players.Count() != leaderboard.size()) {
		for (size_t i = leaderboard.size(); i >= 1; i--)
		{
			bool remove = true;
			for (size_t j = 0; j < currentNames.size(); j++)
			{
				if (currentNames[j] == leaderboard[i - 1].name && currentUids[j] == leaderboard[i - 1].uid.GetIdString()) {
					remove = false;
					break;
				}
			}
			// if (remove) { player_ranks.erase(leaderboard[i - 1].uid.GetIdString()); leaderboard.erase(leaderboard.begin() + i - 1); }
		}
	}
}

bool PlatformDisplay::compareName(int mmr1, std::string name1, int mmr2, std::string name2) {
	if (mmr1 < mmr2) return true;
	else if (mmr1 == mmr2) {
		return to_lower(name1).compare(to_lower(name2)) == -1;
	}
	else return false;
}



std::string PlatformDisplay::to_lower(std::string s) {
	std::for_each(s.begin(), s.end(), [this](char& c) {
		c = std::tolower(c);
		});
	return s;
}

void PlatformDisplay::Render(CanvasWrapper canvas) {
	CVarWrapper enabledCvar = cvarManager->getCvar("PlatformDisplay_Enabled");
	if (!enabledCvar) { return; }
	bool enabled = enabledCvar.getBoolValue();
	if (show && enabled) {
		//get the pos and color for the blue team

		//cvars
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

		//-----Black Magic, thanks BenTheDan------------
		Vector2 canvas_size = gameWrapper->GetScreenSize();
		if (float(canvas_size.X) / float(canvas_size.Y) > 1.5f) scale = 0.507f * canvas_size.Y / SCOREBOARD_HEIGHT;
		else scale = 0.615f * canvas_size.X / SCOREBOARD_WIDTH;

		uiScale = gameWrapper->GetDisplayScale();
		// LOG("Got UI scale {}", uiScale);
		mutators = mmrWrapper.GetCurrentPlaylist() == 34;
		Vector2F center = Vector2F{ float(canvas_size.X) / 2, float(canvas_size.Y) / 2 };
		float mutators_center = canvas_size.X - 1005.0f * scale * uiScale;
		if (mutators_center < center.X && mutators) center.X = mutators_center;
		int team_difference = num_blues - num_oranges;
		center.Y += IMBALANCE_SHIFT * (team_difference - ((num_blues == 0) != (num_oranges == 0)) * (team_difference >= 0 ? 1 : -1)) * scale * uiScale;

		image_scale = 0.48f;
		float tier_X = -SCOREBOARD_LEFT - IMAGE_WIDTH * image_scale;
		float tier_Y_blue = -BLUE_BOTTOM + (6 * (4 - num_blues));
		float tier_Y_orange = ORANGE_TOP;
		int div_X = int(std::roundf(center.X + (-SCOREBOARD_LEFT - 100.0f * image_scale) * scale * uiScale));
		image_scale *= scale * uiScale;

		// based on 100x100 img
		//------End Black Magic---------
		int blues = -1;
		int oranges = -1;
		for (auto pri : leaderboard) {
			if (pri.team == 0) {
				blues++;
			}
			else if (pri.team == 1){
				oranges++;
			}
			if (pri.isBot || pri.team > 1) {
				continue;
			}

			std::string playerName = pri.name; // "playername"
			std::string playerOS = PlatformMap.at(pri.platform); // "os"
			std::string playerString = playerOS + playerName; // "[OS]playername"

			float Y;
			if (pri.team == 0) { Y = tier_Y_blue - BANNER_DISTANCE * (num_blues - blues) + 9; }
			else { Y = tier_Y_orange + BANNER_DISTANCE * (oranges); }
			float X = tier_X + 100.0f * 0.48f + 31;

			Y = center.Y + Y * scale * uiScale;
			X = center.X + X * scale * uiScale;
			canvas.SetPosition(Vector2{ (int)X, (int)Y });
			//canvas.FillBox(Vector2{(int)(100.0*image_scale),(int)( 100.0*image_scale)});

			// 2x scale
			//canvas.DrawString(playerString, 2.0, 2.0);
			int platformImage = pri.platform;
			std::shared_ptr<ImageWrapper> image = (doOverride == 1)? notintlogos[PlatformImageMap[platformImage]] : logos[PlatformImageMap[platformImage]];
			if (image->IsLoadedForCanvas()) {
				canvas.SetPosition(Vector2{ (int)X, (int)Y });
				if (doOverride == 1) {
					canvas.SetColor({ 255, 255, 255, 255 });
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
						canvas.DrawTexture(image.get(), 100.0f / 48.0f * image_scale); // last bit of scale b/c imgs are 48x48
						//if steam player
					} else if(steamPlayerCvar.getBoolValue() && pri.platform!=1) {
						canvas.DrawTexture(image.get(), 100.0f / 48.0f * image_scale); // last bit of scale b/c imgs are 48x48
					}
			}
			else {
				LOG("not loaded for canvas");
			}
		}
	}
}
void PlatformDisplay::getNamesAndPlatforms() {
	//reset the vectors so it doesnt grow
	BlueTeamValues.clear();
	OrangeTeamValues.clear();
	orangeTeamLogos.clear();
	blueTeamLogos.clear();

	//get server
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { return; }

	//get the cars in the sever
	ArrayWrapper<CarWrapper> cars = server.GetCars(); // here

	//for car in {}
	for (CarWrapper car : cars) {


		// idk what a pri is but we get it here
		PriWrapper pri = car.GetPRI();
		if (!pri) { return; }

		//get the uniqueID and the platform -> int <0-13>
		UniqueIDWrapper uniqueID = pri.GetUniqueIdWrapper(); //here
		OnlinePlatform platform = uniqueID.GetPlatform(); //here
		if (!platform) { return; }

		// declare in iterator and an os string
		std::map<int, std::string>::iterator it;
		std::string OS;

		//wanting to check what team the player is on so we can add them to the right vector
		int teamNum = car.GetTeamNum2();
		//get the owner name
		std::string name = car.GetOwnerName();
		//for len
		for (it = PlatformMap.begin(); it != PlatformMap.end(); it++) {
			
			int key = it->first; //int <0-12>

			if (key == platform) {
				OS = it->second; // what ever the number is mapped to
				break;
			}
		}
		//add correspondingly with the name and os
		if (teamNum == 0) { BlueTeamValues.push_back({ name, OS }); blueTeamLogos.push_back(platform); }
		if (teamNum == 1) { OrangeTeamValues.push_back({ name, OS }); orangeTeamLogos.push_back(platform); }

	}
}
