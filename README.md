<img src="https://i.imgur.com/hoU9hex.png" align="right" alt="Headerlogo" height="auto" width="400px">

# DayZ Vanilla++ Mod v0.4

**Last update: **

**Join our Discord server:[Invite Link V++](https://discord.gg/pb27wYh)**

## 🚩 Table of Contents
- [What is Vanilla++](#what-is-vanilla)
- [Installation](#installation)
	- [Client](#client)
	- [Server](#server)
        - [ViLAYER](#vilayer)
	    - [Manual](#manual)
- [Configuration](#configuration)
- [Features](#features)
    - [Spawn Menu](#spawn-menu)
    - [Admin Tools](#admin-tools)
    - [Client Utilities](#client-utilities)
    - [Server Events](#server-events)
	- [Enable/Disable Stamina](#enabledisable-stamina)
	- [Spawn Custom Buildings](#spawn-custom-buildings)
	- [Chatbased Admin Tools](#chatbased-admin-tools)
	- [Player Score/Kill Activity Tracker](#player-scorekill-activity-tracker)
	- [Chatbased Kill Messages](#chatbased-kill-messages)
- [Images](#images)
- [Credits](#credits)

## What is Vanilla++:

Vanilla++ is a DayZ workshop add-on which aims to extend the Vanilla experience by implementing features, and tools which help communities customize their expirence while still allowing for that vanilla DayZ feel.

## Installation:

### Client:

- If you are a player and want to play on Vanilla++ servers, please subscribe to [our steam workshop release](https://steamcommunity.com/sharedfiles/filedetails/?id=1578593068), as well as,  our dependency [RPCFramework](https://steamcommunity.com/workshop/filedetails/?id=1559212036)

- Use the DayZ Launcher, or by changing your startup following start-up commandlines to include:
``` -mod=@VanillaPlusPlus;@RPCFramework ```

### Server:

- [Download Vanilla++](https://github.com/Da0ne/DZMods/archive/master.zip)
- Go into the servers root folder and unpack the folders ```@VanillaPlusPlus``` and ```mpmissions``` there.
- Go inside of the ```@VanillaPlusPlus``` folder, and copy the contents of the ```Keys``` folder into the ```keys``` folder located in your server's root folder.
- Create a new folder called ```server_files``` and copy your server config into there (usually called```serverDZ.cfg```)
- Open the copied config file and change 'dayzOffline.chernarusplus' to 'DayZSurvival.chernarusplus'
- Subscribe to [RPCFramework](https://steamcommunity.com/workshop/filedetails/?id=1559212036) on the Steam workshop.
- On your DayZ Launcher, go into the ```Mods``` right click ```RPCFramework```, and select open folder. If you are prompted with a warning, click I understand. Copy the ```@RPCFramework``` folder to your servers root folder.
- Go inside the copied folder and copy it's ´JacobMango.bikey´ file into the server's keys folder, located in the server root folder.
- Change your startup following start-up commandlines to include
``` -mod=@VanillaPlusPlus;@RPCFramework -config=server_files\serverDZ.cfg -profiles=server_files -name=myServerName -scrAllowFileWrite ```

#### ViLAYER:

For those who own a server provide by [Vilayer.com](https://www.Vilayer.com), can auto install the mod via the `AddonManager`.</br>Special thanks goes to the Vilayer team!

<img src="https://ci5.googleusercontent.com/proxy/yE1RaRsPdehe3Y5lTBg3K2UmKkKYBPUl4HOLneY8hzalp34EayGB0c8qeUxsz4W_vaGETV57DzOYC1huj7bvJmDlyfIodhIE2p07uyLeVuvkyCvCOmKg=s0-d-e1-ft#https://www.vilayer.com/templates/ColoNode/html/img/logo_dark.png" alt="vilayer logo" width="auto" height="70px">

## Configuration:


## Features:

<img src="https://camo.githubusercontent.com/5863adbb631a578b2bf329922ebcd25008bb4a3c/68747470733a2f2f737465616d75736572696d616765732d612e616b616d616968642e6e65742f7567632f3936333130343532383634333232353537312f314530433131333746394536324642443744433532443846323345354641313034463733334546332f3f696e746572706f6c6174696f6e3d6c616e637a6f732d6e6f6e65266f75747075742d666f726d61743d6a706567266f75747075742d7175616c6974793d3935266669743d696e7369646525374336333725334133353826636f6d706f736974652d746f3d2a2c2a253743363337253341333538266261636b67726f756e642d636f6c6f723d626c61636b" alt="ingamemap" height="auto" width="400px">
   
<img src="https://camo.githubusercontent.com/f3dd43dbec49013bfca34e551292e0a14dcd7cb3/68747470733a2f2f737465616d75736572696d616765732d612e616b616d616968642e6e65742f7567632f3936333130343532383634333232363336382f323133334345463737413530363837323731453130463236433539314238363532323645414435332f3f696e746572706f6c6174696f6e3d6c616e637a6f732d6e6f6e65266f75747075742d666f726d61743d6a706567266f75747075742d7175616c6974793d3935266669743d696e7369646525374336333725334133353826636f6d706f736974652d746f3d2a2c2a253743363337253341333538266261636b67726f756e642d636f6c6f723d626c61636b" alt="ingamemap" height="auto" width="400px">

#### Spawn Menu:
- Spawn Points:
 - Can be configured to allow players to pick spawn, or choose randomly; also, can force only random spawn points.
 - Cooldown System:
  - Gives Admins control over how often a player can use a given spawnpoint, can be disabled to not have a cooldown.
- Loadouts:
   - Static:
    - Allows the player to pick from a list of premade loadouts.
   - Advanced:
    - Allows the player to pick items from a list for each slot on the player.
   - Normal:
    - Spawns user with default clothing, and items.

#### Admin Tools:
**Only Admin with valid permissions will be able to access each section of the menu. SuperAdmin's can change these permissions.**
- Object Manager:
  - Gives the ability to place, delete, and save building, and world objects in the world for later loading.
   - No restart is needed.
- Item Manager:
  - Give the ability to spawn items inside of the world.
- Weather Manager:
  - Gives the ability to change weather conditions on the fly, and change their chances.
- Player Manager:
  - Give's the ability to call commands on other players on the server.
   - Commands: To be determined...
- Server Manager:
  - Give's the ability to edit server xml files on the fly, and in-game.
 
#### Client Utilities:

<img src="https://camo.githubusercontent.com/7d5c3d78885238d604afaf40e8f716d828a6430a/68747470733a2f2f737465616d75736572696d616765732d612e616b616d616968642e6e65742f7567632f3936333130343532383634333232313332362f443734433742423243413238393143393531383639433637394633463444313344453137374536392f3f696e746572706f6c6174696f6e3d6c616e637a6f732d6e6f6e65266f75747075742d666f726d61743d6a706567266f75747075742d7175616c6974793d3935266669743d696e7369646525374336333725334133353826636f6d706f736974652d746f3d2a2c2a253743363337253341333538266261636b67726f756e642d636f6c6f723d626c61636b" alt="ingamemap" height="auto" width="400px">

- In-game Map:
  - This gives everyone an in-game map with the ability to place markers. It also shows the player where they are currently located at.
 
#### Server Events:
- Infected Hordes:
  - Spawns hordes of zombies at a configurable time interval, has the ability to have more than one active horde at a time.
- Supply Drop:
  - Spawns a supply plane at a configurable time interval, and drops off a supply crate with configurable loot.
 
#### Customized loadouts using the AdminTool:
- This feature allows the server owner to create loadouts for fresh spawns to spawn with by adding your desired items to \ScriptedMods\LoadOuts\RandomlyGenerated OR Enable Static Loadouts via the Tuneables.c.

- Use the /export loadoutname command via the AdminTools to added a preset players can choose to spawn with. (Only used with Static Loadouts)

#### Enable/Disable Stamina:
- Add ´-DisableStamina´ to your startup commandline to enable, and remove it from your startup commandline to disable.

#### Chatbased Admin Tools:
- This feature allows the server owner to add Steam 64 IDs into a text file, allowing those users to login using the password set inside of the server.cfg. This file is located in \DayZSurvival.chernarusplus\ScriptedMods\Admins.txt

- An admin can login by typing the following into DayZ's chat system: ```#login mypassword```

#### Player Score/Kill Activity Tracker:
- This feature creates two files into the servers profile, and stores detailed kill information and player stats such as total deaths, and kills.

#### Chatbased Kill Messages:
- Sends chat messages showing who killed who, and who committed suicide. (UI Coming soon.)

## Credits:

- @Sen ( @zalexki zalexki ) from DayZModders Discord Server for the KillScore system https://github.com/zalexki
- @Slluxx for teaching @GravityWolfNotAmused how not to suck at readme.md files.
- Github User Moondarker for /refuel cmd added, /spawncar modified to spawn car in dir of admin view (Admin Tools)
