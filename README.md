

<img src="https://i.imgur.com/3bu7aN9.png" align="right" alt="Headerlogo" height="128px" width="128px">

# DayZ Vanilla++ Admin Tools v1.1

## Table Of Contents:
- [What is Vanilla++](#what-is-vanilla)
- **Installation:**
	- [Client](#adding-mod-to-client)
	- [Server](#adding-mod-to-server)
		- [Formatting Json Files](#manual-formatting-of-json-files)
		- [Adding Super Admin](#adding-a-super-admin)
		- [End](#end)
	- [Donation Information](#donation-information)
	- [Contact us](#contact-us)

**Patch Notes:**

- [1.0.1](https://github.com/VanillaPlusPlus/VPP-Admin-Tools/releases/tag/1.0.1)

- [1.0.2](https://github.com/VanillaPlusPlus/VPP-Admin-Tools/releases/tag/1.0.2)

- [1.1](https://github.com/VanillaPlusPlus/VPP-Admin-Tools/releases/tag/v1.1)

## What is VPPAT (Vanilla++ Admin Tools):

VPPAT is a DayZ workshop add-on which aims to add administration tools by implementing a collection of features which help communities manage their players while keeping the installation, and configuration simple for new, and advanced users.

## Adding Mod to Server

First, subscribe to the dependencies: [VPPAdminTools](https://steamcommunity.com/sharedfiles/filedetails/?id=1708571078), [CF](https://steamcommunity.com/workshop/filedetails/?id=1559212036), [VPPNotifications](https://steamcommunity.com/sharedfiles/filedetails/?id=1680673106) on the Steam Workshop.

Second, launch the vanilla DayZ launcher, not DZSA Launcher, and in the bottom of the window it should verify that you just recently subscribed, and downloaded the mods. If not, then check with your steam client and make sure its downloading workshop content on the download page. Using the DayZ Launcher, navigate to the mods tab, search for and right click on **VPPAdminTools**, and click on Open Folder.
<img src="https://i.imgur.com/VQDwxIQ.png" align="bottom" alt="img0" height="458px" width="1215px">
Copy the folder **@VPPAdminTools** to your server root directory, now open the copied files and navigate to the **Keys** folder. Copy all content and place them in the **Keys** folder that is in the root of your server directory. Next, you will have to add these mods to your start up **command line**. By adding: **-mod=@CF;@VPPNotifications;@VPPAdminTools** to your start up command line (Server). Please make sure that your last mod doesn't end with a semicolon. Mod load order is NOT required however, if you run into **Virtual Machine Error** on startup then edit your command line to load dependency mods first before **@VPPAdminTools**
**Redo the same steps above with the two other mods, @CF + @VPPNotifications**

**IMPORTANT: The steps above can be automated if you are renting from a GSP / Game Service Provider**
Many hosting companies have different methods to adding mods to their server, and you will have to figure out the preferred way to add them to your server. Each hosting company is different. Some have auto mod installer, others provide you access to FTP of your root directory. In that case you will need to transfer the mods manually ( use FileZilla FTP or any client of your choice ), you will need to make sure you have the **Keys** of each mod added to the **Keys** directory in the root of the server and ensure your command line has the mod parameters added.

## Profiles folder
The following step is **very important** in getting the configuration for the admin tools to generate.
If you have not already setup a profiles folder for your server, then you need to preform the following:
Add **-profiles=AFolderName** to your server startup  parameters, replace **AFolderName** with anything you wish, on server startup that folder will generate in the root directory of your server. It will contain all of your server logs and any mod generated configuration files.

**IMPORTANT:** If you are renting from a GSP / Game Service Provider, the steps above are not needed! The server is already configured to preform this step, however you will need to locate the profiles folder so you can access the mod generated configuration files! ( The configuration files generate after **1st startup** of server with the mods loaded ). Using your FTP access or a File Manager feature with your GSP, search for a folder that contains files of types **.RPT .LOG .ADM** within that folder you will find the configuration of the admin tools. Keep in mind, the configuration is generated after **1st startup** with the mod being installed.

## Adding a Super Admin
The only file which needs to be manually edited on the server is the **SuperAdmins.txt** file, which is located in your **Profiles** Folder where your server logs are generated and stored. Make sure to start the server after installing the mod to generate the configuration, shut it down once generated. Inside the profiles folder you should find: **VPPAdminTools/Permissions/SuperAdmins**. Inside of this folder the **SuperAdmins.txt** exists. Open it and append your **Steam64** ID. You can fetch your ID using the following [website](https://steamid.io/) paste your steam profile **URL** into the search box and click **lookup** copy the **Steam64** ID to your clipboard and paste it into the **SuperAdmins.txt** save & exit.
<img src="https://i.imgur.com/Ov6nQB3.jpg" align="bottom" alt="img0" height="324px" width="569px">

***Notes:*** You can add multiple IDs, simply append each ID you add to a new line. No spaces or commas / semicolon needed. There is a permission system hierarchy in-place this can be configured by any **SuperAdmin** ingame using the **Permissions Manager Menu** you can create groups that have specific permissions / abilities and also targeting other admins can be adjusted e.g: lower rank admins can NOT target and or execute any commands on a SuperAdmin, however a superadmin can. Edit the **Permission level** of the user group of your choice all available in-game within the admin panel.

## Adding Mod to Client
Subscribe to [VPPAdminTools](https://steamcommunity.com/sharedfiles/filedetails/?id=1708571078), [CF](https://steamcommunity.com/workshop/filedetails/?id=1559212036) and [VPPNotifications](https://steamcommunity.com/sharedfiles/filedetails/?id=1680673106) on the Steam Workshop. Steam will force you to download & load the dependencies.

launch the DayZ Launcher. Go to the mod tab, and select CF, VPPNotifications and VPPAdminTools. Click launch, and play.

You can also use the vanilla DayZ Launchers' **server tab** to find and join your server with auto mod loading.
or you can use DZSA Launcher if your server is configured to run that.

## Keybinds
You need to configure your keybinds via DayZ keybinds settings menu ingame to use the tools.

## Manual Formatting of Json Files
If you wish to manual edit these Json Configuration Files, please use [JsonEditorOnline](https://jsoneditoronline.org/); however, all configuration, other than setting players as super admin, can be done in-game through the UI with the proper permission group, or by being a super admin.

## Object Builder Controls
### -Left Ctrl to deselect an object
### -Hold left Shift + left/right click to rotate object
### -Hold left Alt + left/right click to move up/down
### -Left Click on object to select
### -Hold left Click to drag/move object.

## Donation Information
- [DaOne](https://www.paypal.me/duhonez)
- [GravityWolf](http://paypal.me/GravityWolf)

## FAQ & Troubleshooting
***Q:*** I Press the keybind but nothing happens?

***A:*** Make sure you have your ***Steam64ID*** added as a superadmin under ***VPPAdminTools/Permissions/SuperAdmins/SuperAdmins.txt***

***Q:*** I can't find the profiles folder

***A:*** The "Profile" folder can be different to each Game Service Provider, It contains all your log files of types ( .log, .rpt, .adm ). If you are hosting the server locally, then make sure to follow the instructions carefully [Adding Super Admin](#adding-a-super-admin)

***Q:***  I found the profile folder, but there is no VPPAdminTools folder.

***A:*** Make sure the mod is actually installed on your server, verify that via your GSP Panel or check an .RPT log headline for the mods name. The folder VPPAdminTools is auto-generated during 1st boot of server after installation of mod.

***Q:***  Why is the Server not starting? I'm getting a Virtual Machine Error.

***A:*** This is either caused by **missing required mods, CF / VPPNotifications** OR compatability issues with other admin tool mods ( uninstall other admin tool mods ).

***Q:*** Why do i get kicked for missing pbo or client contains pbo which server rejected.

***A:*** The mod comes with a folder named 'Keys' you need to copy the contents of that folder and paste it into a folder named 'Keys' located in the root directory of your DayZ Server.

**Still having issues? Join our [Discord]([https://discordapp.com/invite/GWXN2bB](https://discordapp.com/invite/GWXN2bB)) server and chat to use about the issues you are running into so we can help you out :D**

## Contact Us
**- Join our [Discord]([https://discordapp.com/invite/GWXN2bB](https://discordapp.com/invite/GWXN2bB)) Server**
**- Email: vanillaplusplusdayz@gmail.com**

## End
**IMPORTANT NOTICES:**  
**- You are not allowed to repack or republish this mod on any platform including steam a DMCA will be filed if necessary**  
**- You are allowed to unpack this mod and create addons to publish on the SteamWorkshop using the Steam Workshop mod dependency feature and Enforce-Engine modding / overriding capabilities.**
