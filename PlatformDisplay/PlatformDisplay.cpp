#include "pch.h"

#include "PlatformDisplay.h"

#include<functional>
#include <map>
#include <iostream>
#include <unordered_set>
#include "ScoreboardPositionInfo.h"

/* TODO
	
	Done: Add tintable icons
	Done: Add offset for unique positioning | remove interference with RLProfilePictures
	Done: Remove spectate bug | offset gets added in render but user not on scoreboard
	Done: remote scoreboard offsets	

	Fix Gui code (wtf was i doing)	
	re-add non tinted colored icons
	re-do position logic (cut away from old math)
	re-write everything (its fine but its messy)

*/




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

	ScoreboardOffsets ScoreboardPos;
} // namespace

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
void PlatformDisplay::DefaultSettings() {
	auto file = std::ofstream(saveFile);
	settings.Enabled = true;

	settings.CustomTint = false;
	settings.ARGBTintBlue = -15109121; // default blue color
	settings.ARGBTintOrange = -3972071; // default orange color
	settings.AlphaConsole = false;
	settings.selected = 2;
	settings.SteamPlayer = false;
	settings.Offset = false;
	settings.offsetX = 0;
	settings.offsetY = 0;
	if (file.is_open()) {
		WriteSettings();
	}
	file.close();
}
void PlatformDisplay::onLoad()
{
	_globalCvarManager = cvarManager;
	if (!std::filesystem::exists(saveFile)) {
		std::filesystem::create_directories(saveFile.parent_path());
		DefaultSettings();
	}
	UserIcon = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / "UserIcon.png", false, false);
	UserIcon->LoadForImGui([](bool success) {
		if (success) LOG("[PlatformDisplay] Loaded UserIcon");
		else LOG("[PlatformDisplay] Couldnt Load UserIcon for ImGui");
	});

	for (int i = 0; i < LOGO_COUNT; i++) {
		SquareLogos.push_back(std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / "square" / (std::to_string(i) + ".png"), false, false));
		SquareLogos.at(i)->LoadForCanvas();
	}
	for (int i = 0; i < LOGO_COUNT; i++) {
		CircleLogos.push_back(std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / "circle" / (std::to_string(i) + ".png"), false, false));
		CircleLogos.at(i)->LoadForCanvas();
	}
	for (int i = 0; i < LOGO_COUNT; i++) {
		FullLogos.push_back(std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / "full" / (std::to_string(i) + ".png"), false, false));
		FullLogos.at(i)->LoadForCanvas();
	}

	Logos.push_back(SquareLogos);
	Logos.push_back(CircleLogos);
	Logos.push_back(FullLogos);

	LoadSettings();


	CurlRequest SBO_CRL;
	SBO_CRL.url = "https://raw.githubusercontent.com/SoulDaMeep/PlatformDisplay/refs/heads/master/ScoreboardLookUp.txt";
	HttpWrapper::SendCurlRequest(SBO_CRL, [=](int code, std::string result)
	{
		// Remotely gets offsets to scoreboard so we dont have to update 3 plugins every time they do some bitchery
		if (code != 200) return;
		ScoreboardPos = ParseScoreboardOffsets(result);
		LOG("Parsed Offsets: SCOREBOARD_LEFT = {}, BLUE_BOTTOM = {}", ScoreboardPos.SCOREBOARD_LEFT, ScoreboardPos.BLUE_BOTTOM);
	});

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
// Settings management with Nlohmann
void PlatformDisplay::LoadSettings() {
	auto file = std::ifstream(saveFile);
	nlohmann::json json;
	if (file.is_open()) {
		file >> json;
	}
	// Updates the json file for users who already have it.
	if (!json.contains("Offset"))
	{
		LOG("Loading Defaults");
		DefaultSettings();
		WriteSettings();
	}

	file.close();
	settings = json.get<Settings>();
	
}

void PlatformDisplay::WriteSettings() {
	auto file = std::ofstream(saveFile);
	nlohmann::json json = settings;
	if (file.is_open()) {
		file << json.dump(4);
	}
	file.close();
}
// 
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
		if (disconnectedPris.find(nameAndId(pri)) != disconnectedPris.end()) {
			pri.ghost_player = true;
		}

		if (pri.team == 0) {
			if (!pri.isSpectator) numBlues++;
		}
		else if (pri.team == 1) {
			if (!pri.isSpectator) numOranges++;

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
	teamHistory[nameAndId(a)] = teamNumA;

	auto teamNumB = b.GetTeamNum2();
	teamHistory[nameAndId(b)] = teamNumB;
}

void PlatformDisplay::SetTeamColors(bool keepOrder) {
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper sw = gameWrapper->GetCurrentGameState();
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

	LinearColor blueColor = teamColors[0];
	LinearColor orangeColor = teamColors[1];
	if (settings.CustomTint)
	{
		// to LinearColor
		blueColor = intToColor(settings.ARGBTintBlue) * 255;
		orangeColor = intToColor(settings.ARGBTintOrange) * 255;
	}
	auto logoList = Logos.at(settings.selected);

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	Vector2 screenSize = gameWrapper->GetScreenSize();
	Vector2F screenSizeFloat{ screenSize.X, screenSize.Y };
	SbPosInfo sbPosInfo = getSbPosInfo(screenSizeFloat,
									   gameWrapper->GetDisplayScale() * gameWrapper->GetInterfaceScale(),
									   /* mutators= */ mmrWrapper.GetCurrentPlaylist() == 34,
									   computedInfo.bluePlayerCount,
									   computedInfo.orangePlayerCount, ScoreboardPos);

	int blues = -1;
	int oranges = -1;

	Vector2F imageShift = { 0, 0 };
	switch (settings.selected) {
		case 0:
			imageShift = { 0, 0 };
			break;
		case 1:
			imageShift = { 5 * sbPosInfo.profileScale, -5 * sbPosInfo.profileScale };
			break;
		case 2:
			imageShift = { 0 , 0 };
			break;
	}
	// shift by offset relative to scoreboard position
	if(settings.Offset)
		imageShift = {sbPosInfo.profileScale * settings.offsetX, sbPosInfo.profileScale * settings.offsetY};

	for (auto pri : computedInfo.sortedPlayers) {
		// user is not on scoreboard
		// is spectating or user hasnt selected team yet. (select team menu)
		if (pri.isSpectator || pri.team == 255)
			continue;

		Vector2F drawPos{};
		// blue
		if (pri.team == 0) {
			blues++;
			canvas.SetColor(blueColor);
			if (pri.ghost_player) canvas.SetColor(LinearColor{ blueColor.R, blueColor.G, blueColor.B, 155 / 1.5 });
			drawPos = sbPosInfo.blueLeaderPos + Vector2F{ 0, sbPosInfo.playerSeparation * blues } + imageShift;
		}
		// orange
		else if (pri.team == 1) {
			oranges++;

			canvas.SetColor(orangeColor);
			if (pri.ghost_player) canvas.SetColor(LinearColor{ orangeColor.R, orangeColor.G, orangeColor.B, 155 / 1.5 });
			drawPos = sbPosInfo.orangeLeaderPos + Vector2F{ 0, sbPosInfo.playerSeparation * oranges } + imageShift;
		}
		if (pri.isBot) { continue; }


		// if the users platform is steam and hide steam setting is on
		if (pri.platform == OnlinePlatform_Steam && settings.SteamPlayer) { continue; }
		// if alphaconsole setting is on and the current render pri is yourself.
		if (settings.AlphaConsole && (pri.uid == gameWrapper->GetUniqueID())) { continue; }

		canvas.SetPosition(drawPos);

		std::shared_ptr<ImageWrapper> image = logoList[PlatformImageMap[pri.platform]];
		if (!image->IsLoadedForCanvas()) {
			LOG("[Image] Image not loaded for canvas.");
			continue;
		}

		canvas.DrawTexture(image.get(), 100.0f / image.get()->GetSize().X * sbPosInfo.profileScale); // last bit of scale b/c imgs are 48x48		
	}
}
