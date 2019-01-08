<img src="https://i.imgur.com/hoU9hex.png" align="right" alt="Headerlogo" height="auto" width="400px">

# DayZ Vanilla++ Mod v0.4
**DayZ Update 1.0: Fixed issues with mod startup, now functional however "StaticLoadouts/Preset" do not function due to imporper/aka borken core functionality (making folders for data to be housed). So all server owners running this mod, please use "Randomized loadouts" as till we get a fix for this meme. Other broken things include: Custom Player Stats don't get recorded (Kills,deaths) , Can't save spawnpoints from the admin tools, can't export loadouts via the admintools :( **

**Last update: December 1st 2018**

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
    - [In game Map](#in-game-map)
	- [Custom Ladout & Spawn Menu](#custom-ladout--spawn-menu)
	- [Customized loadouts using the AdminTool](#customized-loadouts-using-the-admintool)
	- [Enable/Disable Stamina](#enabledisable-stamina)
	- [Spawn Custom Buildings](#spawn-custom-buildings)
	- [Chatbased Admin Tools](#chatbased-admin-tools)
	- [Player Score/Kill Activity Tracker](#player-scorekill-activity-tracker)
	- [Chatbased Kill Messages](#chatbased-kill-messages)
- [Images](#images)
- [Credits](#credits)

## What is Vanilla++:

Vanilla++ is a DayZ workshop add-on which aims to extend the Vanilla experience by implementing features, and tool which help server owners customize the expirence of their server while still allowing for that vanilla DayZ feel.

For a full list of features  please see the [Features](#).

## Installation:

### Client:

- If you are a player and want to play on Vanilla++ servers, please subscribe to [our steam workshop release](https://steamcommunity.com/sharedfiles/filedetails/?id=1578593068), as well as,  our dependency [RPCFramework](https://steamcommunity.com/workshop/filedetails/?id=1559212036)

- Change your startup following start-up commandlines to include 
``` -mod=@VanillaPlusPlus;@RPCFramework ```
(Alternatively: Enable the mods via the DayZLauncher and click ´play´)

### Server:

#### ViLAYER:

For those who own a server provide by [Vilayer.com](https://www.Vilayer.com), can auto install the mod via the `AddonManager`.</br>Special thanks goes to the Vilayer team!

<img src="https://ci5.googleusercontent.com/proxy/yE1RaRsPdehe3Y5lTBg3K2UmKkKYBPUl4HOLneY8hzalp34EayGB0c8qeUxsz4W_vaGETV57DzOYC1huj7bvJmDlyfIodhIE2p07uyLeVuvkyCvCOmKg=s0-d-e1-ft#https://www.vilayer.com/templates/ColoNode/html/img/logo_dark.png" alt="vilayer logo" width="auto" height="70px">

#### Manual:

- [Download Vanilla++](https://github.com/Da0ne/DZMods/archive/master.zip)
- Go into the servers root folder and unpack the folders ```@VanillaPlusPlus``` and ```mpmissions``` there.
- Create a new folder called ```VanillaPlusPlus_files``` and copy your server config into there (usually called```serverDZ.cfg```)
- Open the copied config file and change 'dayzOffline.chernarusplus' to 'DayZSurvival.chernarusplus'
- Subscribe to [RPCFramework](https://steamcommunity.com/workshop/filedetails/?id=1559212036) on the Steam workshop.
- On your DayZ client, go into the ```!workshop``` folder and copy ```@RPCFramework``` to the root of the server (just like in step 2). If the folder is hidden, open the dayz launcher, go to ´mods´, rightclick ´RPCFramework´ and click on ´open folder´
- Go inside the copied folder and copie its ´keys´ folder into the server root folder.
- Change your startup following start-up commandlines to include 
``` -mod=@VanillaPlusPlus;@RPCFramework -config=VanillaPlusPlus_files\serverDZ.cfg -profiles=VanillaPlusPlus_files -name=myServerName -scrAllowFileWrite ```

## Configuration:

- In order to turn on, or off different functionality, check the top of: ```'/mpmission/DayZSurvival.chernarusplus/ScriptedMods/Tunables.c'```

- In order to change which spawn menu play will interact with, check the top of:
```'/mpmission/DayZSurvival.chernarusplus/ScriptedMods/Modules/AdvancedLoadouts.c'```

## Features:

#### In-game Map:
- Just as the header suggests, we created a in-game map for players to use.

#### Custom Ladout & Spawn Menu:
- This feature allows the server to create loadouts for fresh spawns to spawn with. The server owner can also set spawn points around the map.

#### Customized loadouts using the AdminTool:
- This feature allows the server owner to create loadouts for fresh spawns to spawn with by adding your desired items to \ScriptedMods\LoadOuts\RandomlyGenerated OR Enable Static Loadouts via the Tuneables.c.

- Use the /export loadoutname command via the AdminTools to added a preset players can choose to spawn with. (Only used with Static Loadouts)

#### Enable/Disable Stamina:
- Add ´-DisableStamina´ to your startup commandline to enable, and remove it from your startup commandline to disable.

#### Spawn Custom Buildings:
- This feature allows the server owner to place CustomBuildings by minimalizing the amount of code needed to spawn in additional buildings.

#### Chatbased Admin Tools:
- This feature allows the server owner to add Steam 64 IDs into a text file, allowing those users to login using the password set inside of the server.cfg. This file is located in \DayZSurvival.chernarusplus\ScriptedMods\Admins.txt

- An admin can login by typing the following into DayZ's chat system: ```#login mypassword```

#### Player Score/Kill Activity Tracker:
- This feature creates two files into the servers profile, and stores detailed kill information and player stats such as total deaths, and kills.

#### Chatbased Kill Messages:
- Sends chat messages showing who killed who, and who committed suicide. (UI Coming soon.)

## Images

<img src="https://camo.githubusercontent.com/7d5c3d78885238d604afaf40e8f716d828a6430a/68747470733a2f2f737465616d75736572696d616765732d612e616b616d616968642e6e65742f7567632f3936333130343532383634333232313332362f443734433742423243413238393143393531383639433637394633463444313344453137374536392f3f696e746572706f6c6174696f6e3d6c616e637a6f732d6e6f6e65266f75747075742d666f726d61743d6a706567266f75747075742d7175616c6974793d3935266669743d696e7369646525374336333725334133353826636f6d706f736974652d746f3d2a2c2a253743363337253341333538266261636b67726f756e642d636f6c6f723d626c61636b" alt="ingamemap" height="auto" width="400px">
Ingame Map

<img src="https://camo.githubusercontent.com/5863adbb631a578b2bf329922ebcd25008bb4a3c/68747470733a2f2f737465616d75736572696d616765732d612e616b616d616968642e6e65742f7567632f3936333130343532383634333232353537312f314530433131333746394536324642443744433532443846323345354641313034463733334546332f3f696e746572706f6c6174696f6e3d6c616e637a6f732d6e6f6e65266f75747075742d666f726d61743d6a706567266f75747075742d7175616c6974793d3935266669743d696e7369646525374336333725334133353826636f6d706f736974652d746f3d2a2c2a253743363337253341333538266261636b67726f756e642d636f6c6f723d626c61636b" alt="ingamemap" height="auto" width="400px">
Custom loadout & Spawn Menu

<img src="https://camo.githubusercontent.com/f3dd43dbec49013bfca34e551292e0a14dcd7cb3/68747470733a2f2f737465616d75736572696d616765732d612e616b616d616968642e6e65742f7567632f3936333130343532383634333232363336382f323133334345463737413530363837323731453130463236433539314238363532323645414435332f3f696e746572706f6c6174696f6e3d6c616e637a6f732d6e6f6e65266f75747075742d666f726d61743d6a706567266f75747075742d7175616c6974793d3935266669743d696e7369646525374336333725334133353826636f6d706f736974652d746f3d2a2c2a253743363337253341333538266261636b67726f756e642d636f6c6f723d626c61636b" alt="ingamemap" height="auto" width="400px">
Custom loadout & Spawn Menu 2

## Credits:

- @Sen ( @zalexki zalexki ) from DayZModders Discord Server for the KillScore system https://github.com/zalexki
- @Slluxx for teaching @GravityWolfNotAmused how not to suck at readme.md files.
- Github User Moondarker for /refuel cmd added, /spawncar modified to spawn car in dir of admin view (Admin Tools)
