# PlatformDisplay

plugin id: 376 

Big thanks to the team that helped and supported the making of this plugin:

> Images gathered and edited by Swan \
> Images, Banner, and funding by [Mrkz](https://steamcommunity.com/id/Mrkz96/) \
> Implementation of scoreboard images by Rivques \
> Implementation of "Ghost Players" and refactoring by LikeBook 

## "Ghost Players"
<p>
When a player leaves, their username might stay on the scoreboard as a dimmed-out version of their user; we We like to call these "Ghost Players". Unfortunately, this issue has caused significant problems in our plugin, as the leaving of a player messes up the entire scoreboard and disrupts its alignment. We don't know when a player leaves whether or not the scoreboard will keep the user as a Ghost Player, but we can take data from RocketLeague itself to decide whether or not the player will stay as a Ghost Player. 
</p>

More information [here](PlatformDisplay/GhostPlayers.md)

## Start of game sorting
<p>
You may have noticed that when the game starts and you're on the countdown, pressing the scoreboard button reveals that your platform doesn't match your actual platform. This discrepancy arises from our lack of knowledge regarding the precise sorting algorithm employed by Rocket League. Currently, we sort players based on their scores. Consequently, when two players have the same score, or at the beginning of the game where everyone's score is "0", our plugin struggles to determine the correct order. However, rest assured that the plugin will rectify this issue as soon as players begin accumulating points.
</p>

