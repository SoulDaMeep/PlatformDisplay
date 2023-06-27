# Ghost Players

When a player leaves the game, they have a chance to stay on the scoreboard but dimmed out; This is what we call "Ghost Players".
We do not know the RocketLeague way of telling whether or not the player will stay on the scoreboard after leaving or not. But we can use ingame wrappers to aid us in finding the players still on the scoreboard.

These function hooks were discovered by LikeBook:
```cpp
Function TAGame.GameEvent_Soccar_TA.ScoreboardSort
struct Params {
  uintptr_t PRI_A;
  uintptr_t PRI_B;

  // if hooking post
  int32_t ReturnValue;
};

Function TAGame.GFxData_Scoreboard_TA.UpdateSortedPlayerIDs

Function TAGame.PRI_TA.GetScoreboardStats
struct ScoreboardStat {
    // FName use, gameWrapper->GetFnameByIndex(name_id)
    int32_t name_id;
    int32_t _instance_number;

    int32_t value;
};
struct Params {
  // Only potentially available when hooking post.

  // generic dump of Array used in game that you should be able to use to construct something.
  uintptr_t ArrayData; // ScoreboardStats
  int32_t ArrayCount;
  int32_t ArrayMax;
};
```
We discovered that not alot of this worked. The returnValue in the ScoreBoardSort Function hook did not return anything (Which would of been very helpful). In the ScoreboardStat Function hook, none of these really worked either. We could only use 2 parameters from the ScoreboardSort Function hook. Using the data from the ScoreboardSort Function hook, we could get all the players that would be sorted in the scoreboard, including the Ghost Players. This is how we got the Ghost Players in the scoreboard and we could now treat them like normal users.
