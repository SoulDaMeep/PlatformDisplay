#pragma once

#include "bakkesmod/wrappers/wrapperstructs.h"
#include <unordered_map>
//#define SCOREBOARD_LEFT 537
//
//#define BLUE_BOTTOM 67
//#define ORANGE_TOP 43
//
//#define BANNER_DISTANCE 57
//#define IMAGE_WIDTH 150
//#define IMAGE_HEIGHT 
//#define CENTER_X 960
//#define CENTER_Y 540
//#define SCOREBOARD_HEIGHT 548
//#define SCOREBOARD_WIDTH 1033
//#define IMBALANCE_SHIFT 32
//#define MUTATOR_SIZE 478
//#define SKIP_TICK_SHIFT 67
//#define Y_OFFCENTER_OFFSET 32

// Scoreboard Position Info: 
struct SbPosInfo {
	Vector2F blueLeaderPos;  // position of the profile box for the top blue player
	Vector2F orangeLeaderPos;
	float playerSeparation;  // distance between profile pics on each team
	float profileScale;      // amount to scale each platform icon / profile pic
};

struct ScoreboardOffsets {
	int SCOREBOARD_LEFT = 0;
	int BLUE_BOTTOM = 0;
	int ORANGE_TOP = 0;
	int BANNER_DISTANCE = 0;
	int IMAGE_WIDTH = 0;
	int IMAGE_HEIGHT = 0;
	int CENTER_X = 0;
	int CENTER_Y = 0;
	int SCOREBOARD_HEIGHT = 0;
	int SCOREBOARD_WIDTH = 0;
	int IMBALANCE_SHIFT = 0;
	int MUTATOR_SIZE = 0;
	int SKIP_TICK_SHIFT = 0;
	int Y_OFFCENTER_OFFSET = 0;
};

enum OffsetKey {
	SCOREBOARD_LEFT,
	BLUE_BOTTOM,
	ORANGE_TOP,
	BANNER_DISTANCE,
	IMAGE_WIDTH,
	IMAGE_HEIGHT,
	CENTER_X,
	CENTER_Y,
	SCOREBOARD_HEIGHT,
	SCOREBOARD_WIDTH,
	IMBALANCE_SHIFT,
	MUTATOR_SIZE,
	SKIP_TICK_SHIFT,
	Y_OFFCENTER_OFFSET,
	UNKNOWN
};

const std::unordered_map<std::string, OffsetKey> keyMap = {
	{"SCOREBOARD_LEFT", SCOREBOARD_LEFT},
	{"BLUE_BOTTOM", BLUE_BOTTOM},
	{"ORANGE_TOP", ORANGE_TOP},
	{"BANNER_DISTANCE", BANNER_DISTANCE},
	{"IMAGE_WIDTH", IMAGE_WIDTH},
	{"IMAGE_HEIGHT", IMAGE_HEIGHT},
	{"CENTER_X", CENTER_X},
	{"CENTER_Y", CENTER_Y},
	{"SCOREBOARD_HEIGHT", SCOREBOARD_HEIGHT},
	{"SCOREBOARD_WIDTH", SCOREBOARD_WIDTH},
	{"IMBALANCE_SHIFT", IMBALANCE_SHIFT},
	{"MUTATOR_SIZE", MUTATOR_SIZE},
	{"SKIP_TICK_SHIFT", SKIP_TICK_SHIFT},
	{"Y_OFFCENTER_OFFSET", Y_OFFCENTER_OFFSET},
};

SbPosInfo getSbPosInfo(Vector2F canvasSize, float uiScale, bool mutators, int numBlues, int numOranges, ScoreboardOffsets sbo) {
	//-----Black Magic, thanks BenTheDan------------
	float scale;
	if (canvasSize.X / canvasSize.Y > 1.5f) {
		scale = 0.507f * canvasSize.Y / sbo.SCOREBOARD_HEIGHT;
	}
	else {
		scale = 0.615f * canvasSize.X / sbo.SCOREBOARD_WIDTH;
	}

	Vector2F center = Vector2F{ canvasSize.X / 2, canvasSize.Y / 2 + +sbo.Y_OFFCENTER_OFFSET * scale * uiScale };
	float mutators_center = canvasSize.X - 1005.0f * scale * uiScale;

	if (mutators && mutators_center < center.X) {
		center.X = mutators_center;
	}

	int team_difference = numBlues - numOranges;
	center.Y += sbo.IMBALANCE_SHIFT * (
		team_difference - (
			(numBlues == 0) != (numOranges == 0)
			) * (team_difference >= 0 ? 1 : -1)
		) * scale * uiScale;

	SbPosInfo output;
	output.profileScale = 0.48f;


	float ScoreboardPosX = -sbo.SCOREBOARD_LEFT - (sbo.IMAGE_WIDTH - 100.0f) * output.profileScale + 30.5f; // 30.5f final right offset
	ScoreboardPosX = center.X + ScoreboardPosX * scale * uiScale;

	// avatar positioning

	float BlueAvatarPos = -sbo.BLUE_BOTTOM + (6 * (4 - numBlues)) - sbo.BANNER_DISTANCE * numBlues + 9.0f;
	BlueAvatarPos = center.Y + BlueAvatarPos * scale * uiScale;

	float OrangeAvatarPos = sbo.ORANGE_TOP;
	OrangeAvatarPos = center.Y + OrangeAvatarPos * scale * uiScale;

	// avatar positioning final

	output.blueLeaderPos = { ScoreboardPosX, BlueAvatarPos };
	output.orangeLeaderPos = { ScoreboardPosX, OrangeAvatarPos };

	//

	output.playerSeparation = sbo.BANNER_DISTANCE * scale * uiScale;
	output.profileScale *= scale * uiScale;

	// based on 100x100 img
	//------End Black Magic---------
	return output;
}

ScoreboardOffsets ParseScoreboardOffsets(const std::string& content) {
	ScoreboardOffsets offsets;

	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line)) {
		// give it a haircut idk
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		// this make it go like....nuh uhhhhhhhh
		if (line.empty() || line.starts_with("//")) continue;

		auto delimiterPos = line.find(' ');
		if (delimiterPos == std::string::npos) continue;

		std::string key =   line.substr(0, delimiterPos);
		std::string value = line.substr(delimiterPos + 1);

		// default value to make sure it always has a value...make it extra kwispy
		int intValue = 0;
		try {
			intValue = std::stoi(value);
		}
		catch (const std::invalid_argument&) {
			intValue = 0;
		}

		auto it = keyMap.find(key);
		OffsetKey keyEnum = (it != keyMap.end()) ? it->second : UNKNOWN;

		switch (keyEnum) {
		case SCOREBOARD_LEFT:    offsets.SCOREBOARD_LEFT = intValue; break;
		case BLUE_BOTTOM:        offsets.BLUE_BOTTOM = intValue; break;
		case ORANGE_TOP:         offsets.ORANGE_TOP = intValue; break;
		case BANNER_DISTANCE:    offsets.BANNER_DISTANCE = intValue; break;
		case IMAGE_WIDTH:        offsets.IMAGE_WIDTH = intValue; break;
		case IMAGE_HEIGHT:       offsets.IMAGE_HEIGHT = intValue; break;
		case CENTER_X:           offsets.CENTER_X = intValue; break;
		case CENTER_Y:           offsets.CENTER_Y = intValue; break;
		case SCOREBOARD_HEIGHT:  offsets.SCOREBOARD_HEIGHT = intValue; break;
		case SCOREBOARD_WIDTH:   offsets.SCOREBOARD_WIDTH = intValue; break;
		case IMBALANCE_SHIFT:    offsets.IMBALANCE_SHIFT = intValue; break;
		case MUTATOR_SIZE:       offsets.MUTATOR_SIZE = intValue; break;
		case SKIP_TICK_SHIFT:    offsets.SKIP_TICK_SHIFT = intValue; break;
		case Y_OFFCENTER_OFFSET: offsets.Y_OFFCENTER_OFFSET = intValue; break;
		default: break;
		}
	}

	return offsets;
}