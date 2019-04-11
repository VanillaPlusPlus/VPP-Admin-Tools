
<img src="https://i.imgur.com/Ys3Abet.png" align="right" alt="Headerlogo" height="auto" width="400px">

# DayZ Vanilla++ Admin Tools v0.6

### Update v0.6 Change log: 

**Join our Discord server:[Invite Link V++](https://discord.gg/pb27wYh)**

## 🚩 Table of Contents
- [What is Vanilla++](#what-is-vanilla)
- [Installation](#installation)
	- [Client](#client)
	- [Server](#server)
        - [ViLAYER](#vilayer)
- [Admin Tools](#admin-tools)
- [Credits](#credits)

## What is Vanilla++:

Vanilla++ is a DayZ workshop add-on which aims to add administration tools by implementing a collection of features, and tools which help communities manager their players.

## Installation:

### Client:

- If you are a player and want to play on Vanilla++ servers, please subscribe to [our steam workshop release](https://steamcommunity.com/sharedfiles/filedetails/?id=1578593068), as well as,  our dependency [Community Framework](https://steamcommunity.com/workshop/filedetails/?id=1559212036)

- Use the DayZ Launcher and load @VPPAdminTools, and @CommunityFramework, or by adding the following to your start-up commandline:
```c
-mod=@CommunityFramework;@VPPAdminTools;
```

### Server:


- [Subscribe to workshop addon](https://steamcommunity.com/sharedfiles/filedetails/?id=1578593068) and [Community Framework](https://steamcommunity.com/workshop/filedetails/?id=1559212036)
- Launch DayZ launcher, Find @VPPAdminTools, right click, and click "Open Folder"
- Take the @VPPAdminTools, and @CommunityFramework folders, and copy them to the server's root directory.
- Take the contents of the Keys folder, which is located inside of both CommunityFramework, and VPPAdminTools, and move those files to the Keys folder, located in the server's root directory.
- Change your startup to include the following following start-up commandlines arguments:
``` -mod=@CommunityFramework;@VPPAdminTools; -profiles=serverLogs```

Note: SuperAdmins.txt, and all json files should be located inside of your profiles folder.

#### ViLAYER:

For those who own a server provide by [Vilayer.com](https://www.Vilayer.com), can auto install the mod via the `AddonManager`.</br>Special thanks goes to the Vilayer team!

<img src="https://ci5.googleusercontent.com/proxy/yE1RaRsPdehe3Y5lTBg3K2UmKkKYBPUl4HOLneY8hzalp34EayGB0c8qeUxsz4W_vaGETV57DzOYC1huj7bvJmDlyfIodhIE2p07uyLeVuvkyCvCOmKg=s0-d-e1-ft#https://www.vilayer.com/templates/ColoNode/html/img/logo_dark.png" alt="vilayer logo" width="auto" height="300px">

<img src="https://i.imgur.com/9ZJkD4P.png" alt="KeybindMenu" width="423px" height="299px">

#### KeyBinding Menu:
- Allows each client to specifiy how they wish for the admin menu to be triggered, and other base function. 
Note: Those who don't have admin/super admin permission will not be able to trigger the admin menu, but they can trigger the keybinding menu.

#### Admin Tools:
**Only Admin with valid permissions will be able to access each section of the menu. SuperAdmin's can change these permissions.**
- Object Manager:
  - Gives the ability to place, delete, and save building, and world objects in the world for later loading.
   - No restart is needed.
- Item Manager:
  - Spawn items inside of the world.
- Weather Manager:
  - Gives the ability to change weather conditions on the fly, and change their chances.
- Player Manager:
  - Give's the ability to call commands on other players on the server.
- Server Manager:
  - Edit server XML files on the fly, and in-game.

#### Chatbased Admin Tools + UI Toolset:
- This feature allows the server owner to add Steam 64 IDs into a text file, allowing those users to login using the password set inside of the server.cfg. This file is located in (THIS SHOULD BE USED BY THE HIGHEST ADMIN TIER, YOU CAN SET OTHER PERMISSIONS VIA THE ADMIN TOOLS) in your DayZ Server profile folder (Where your logs are created and stored) file name is: SuperAdmins.txt

- An admin can login by typing the following into DayZ's chat system: ```#login mypassword```

## Credits:

- @Slluxx for teaching @GravityWolfNotAmused how not to suck at readme.md files. I still suck at readme.md files.
- Github User Moondarker for /refuel cmd added, /spawncar modified to spawn car in dir of admin view (Admin Tools)
