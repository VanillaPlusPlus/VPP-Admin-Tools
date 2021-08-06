
<img src="https://i.imgur.com/3bu7aN9.png" align="right" alt="Headerlogo" height="128px" width="128px">

# DayZ Vanilla++ Admin Tools v1.0

## Table Of Contents:
- [What is Vanilla++](#what-is-vanilla)
- **Installation:**
	- [Client](#adding-mod-to-client)
	- [Server](#adding-mod-to-server)
		- [Formatting Json Files](#manual-formatting-of-json-files)
		- [Adding Super Admin](#adding-a-super-admin)
		- [End](#end)
	- [Donation Information](#donation-information)

**Patch Notes Verson 0.7:**
1. Complete Refactor of Module System
	1. Creation of UI Manager for expandability of the tools.
	2. Simplification of the Admin Tools installation documentation.
2. Completely Overhauled Permission System.
3. Completion of Modules:
	1. Weather Manager
	2. Time Manager
	3. Permission Manager
	4. Teleport Manager
	5. Item Manager
	6. Object Placer
4. Layout overhaul on all previous layouts, and new menus.
5. Creation of example external modules. (still Work in Progress)
	1. Infected Horde Plus
	2.  Vanilla Plus Plus Map
	3. Vanilla Spawn Selector
6. Creation of Developer Documentation for 3rd party expansion of tools. (still Work in Progress)

## What is Vanilla++:

Vanilla++ is a DayZ workshop add-on which aims to add administration tools by implementing a collection of features which help communities manage their players while keeping the installation, and configuration simple for new, and advanced users.

## Adding Mod to Server

First, subscribe to the dependencies: [VPPAdminTools](https://steamcommunity.com/sharedfiles/filedetails/?id=1828439124), [CF](https://steamcommunity.com/workshop/filedetails/?id=1559212036) on the Steam Workshop.

Second, launch the normal DayZ launcher, not DayZ SA Launcher, and in the bottom of the window it should verify that you just recently subscribed, and downloaded the mods. Goto the mods tab, right click on VPPAdminTools, and click on Open Folder.

Inside of the newly opened window, Copy both @VPPAdminTools, @CF and @VPPNotifications from the !Workshop folder to your servers Root Directory. Next, you will have to add these mods to your start up command line. The vanilla process of installing mods is by adding -mod=@CF;@VPPNotifications;@VPPAdminTools to your start up command line (Server). Please make sure that your last mod doesn't end with a semicolon.

Many hosting companies have different methods to adding mods to their server, and you will have to figure out the prefered way to add them to your server. Each hosting company is different.

## Adding Mod to Client
First, subscribe to [VPPAdminTools](https://steamcommunity.com/sharedfiles/filedetails/?id=1708571078), [CF](https://steamcommunity.com/workshop/filedetails/?id=1559212036) and [VPPNotifications] (https://steamcommunity.com/sharedfiles/filedetails/?id=1680673106) on the Steam Workshop. Steam will force you to download & load the dependencies.

Second, launch the DayZ Launcher. Go to the mod tab, and select CF, VPPNotifications and VPPAdminTools. Click launch, and play.

**Note:**
Some server owners use DayZ SA launcher, and require you to join the server through the application. Please contact your community staff team for their prefered way to join their server.

## Manual Formatting of Json Files

If you wish to manual edit these Json Configuration Files, please use [JsonEditorOnline](https://jsoneditoronline.org/); however, all configuration, other than setting players as super admin, can be done in-game through the UI with the proper permission group, or by being a super admin.

## Adding a Super Admin & Password Setup

**To add a SuperAdmin;** 
The file SuperAdmins.txt file located in your Profiles folder located at: **VPPAdminTools/Permissions/SuperAdmins**. Within this file you will add steam ID's, these id's are for those who have the highest permissions over UserGroups. SuperAdmins are the only users in which can add other users into UserGroups. In order to make yourself super admin, you will need to add your own Steam 64 ID.

**To setup a password;** 
Navigate to **Profiles/VPPAdminTools/Permissions/credentials.txt** and follow the instructions written in the generated file. **You MUST set a password** otherwise the tool will not function.
Server restart is required to apply changes. 

**Note:**
If you add multiple steam id's, each one has to be on it's own line with no empty spaces at the start or end. You can get your Steam ID: [Here](http://steamid.io/)

## End
If you have followed these steps correctly, you should be able to get in-game, and press the INSERT key to open the tools & login with your set password. Every button for every module will be on the right side of your screen, and can be opened, and closed by simply clicking on it.

## Donation Information
- [DaOne](https://www.paypal.me/duhonez)
- [GravityWolf](http://paypal.me/GravityWolf)
