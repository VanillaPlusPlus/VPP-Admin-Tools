/*
	In-game changelog content (VPP Admin Tools). Newest section first.

	Authoring rules
	- One builder call per physical line (Enforce: a call statement cannot span lines).
	- Rows: Section(version, date), Added, Changed, Fixed, Removed, Known, Paragraph, Bullet, LinkRow(label, url), Gap().
	- Inline colour: {orange} {blue} {green} {yellow} {red} {muted} {white} ... {/}. No nesting; an unclosed span is closed automatically.
	- Do not put the hash sign, angle brackets or double quotes inside the text. A text that is only a VSTR key (starting with the hash sign) is translated.
	- LinkRow only opens http:// or https:// URLs.
	- After adding a section, bump changelogVersion in assets/config.cpp: the window opens with the toolbar again for every admin and their Don't show again choice resets.

	Other mods can add their own section (shown above VPP's; only VPP's changelogVersion drives the auto-open):

	modded class VPPChangelogContent
	{
		override void Build(VPPChangelogBuilder b)
		{
			b.Section("MyMod 1.2", "2026-10-01");
			b.Added("Something new");
			super.Build(b);
		}
	};
*/
class VPPChangelogContent : Managed
{
	void Build(VPPChangelogBuilder b)
	{
		b.Section("1.7", "2026-09-29");
		b.Changed("{orange}Building sets{/}: DayZ 1.30 broke listing the files of a folder, so the server could no longer find saved building sets. It now keeps its own list of every set it saves (BuildingSetJournal.json in the building sets folder) and loads the sets from that list.");
		b.Bullet("{yellow}Action needed once after updating: building sets saved before this update are not loaded until you import them.{/}");
		b.Bullet("{yellow}On the server, open your profile folder, then VPPAdminTools / ConfigurablePlugins / BuildingSetManager. Run ImportBuildingSets.bat there (the server writes it on its first start with this update), then restart the server.{/}");
		b.Bullet("Not on Windows or can't execute batch script? Write the set names into BuildingSetImport.txt in the same folder instead, one per line, and restart. Sets created or saved from now on are added to the list automatically.");
		b.Added("{orange}XML Editor rebuilt{/}: edit the Central Economy files in game. Every save makes a backup first and only changes the lines you edited, so comments and formatting stay as they were.");
		b.Bullet("{orange}TYPES{/}: search and filters, an inspector with a 3D preview, bulk edits, and add, rename, duplicate, move or delete. A filter lists game items that are in no types file yet, ready to register.");
		b.Bullet("{orange}MAP{/}: see where a type can spawn (buildings, event wrecks, trees, infected) as a heat map or pins. Live scan shows what is on the server right now, and its teleport takes you to the exact spot of the item, upper floors included.");
		b.Bullet("{orange}MESSAGES{/}: edit the server messages (on connect, repeating, countdown to a shutdown, at a set time) with a preview.");
		b.Bullet("{orange}EVENTS{/}: edit dynamic events, their spawn positions on a map (click to add, or add at your position) and event groups. Warnings show what the server will do with each event.");
		b.Bullet("Event groups can be built in the {orange}Object Manager{/}: lay the objects out in the world, then send them back to the group.");
		b.Bullet("{orange}SPAWNABLES{/}: edit what spawns on and in items (attachments, cargo, presets) with the real odds, a 3D preview of a roll, and a test spawn in front of you.");
		b.Bullet("{orange}BACKUPS{/}, {orange}FILES{/} and {orange}CREATE{/}: compare, restore and pin backups, see the problems across all economy files, and create new custom economy files that are registered for you.");
		b.Added("New XML Editor permissions for each part: editing types, messages, events and spawnables, backups, the map, live scan and deleting live objects. Groups that already had the XML Editor get the matching ones automatically.");
		b.Added("{orange}WebHooks rebuilt{/}: every message is now a template you can change, and webhooks are no longer limited to Discord.");
		b.Bullet("A new WebHooks menu lists your webhooks with their delivery health ({green}OK{/}, {red}FAIL{/}, OFF, IDLE) and has three tabs: EVENTS, TEMPLATE and SETTINGS.");
		b.Bullet("{orange}EVENTS{/}: switch each event on or off: admin actions, kills, hits, player joined, left, left during logout, logout started or canceled, server status and server started. Filters: PvP only, skip AI, minimum distance, and which admin tools to include or leave out.");
		b.Bullet("{orange}TEMPLATE{/}: change the message of each event in game, with values such as the victim, killer, weapon, distance and positions. VALIDATE checks it and shows a preview, TEST SEND posts a real message, and RESET TO PRESET brings the original back.");
		b.Bullet("Ready-made presets for {orange}Discord{/} and {orange}Lolka{/} (embed or simple text), Slack, Microsoft Teams, Telegram, generic JSON and plain text.");
		b.Bullet("{orange}SETTINGS{/}: name, URL, preset, messages per minute, hiding Steam IDs or the server address, and your own values such as the bot name and avatar.");
		b.Bullet("Messages are queued and paced to each service's limits, and failed ones are tried again. The health bar shows delivered and failed messages and the last error.");
		b.Bullet("Existing webhooks are converted automatically and keep sending the same messages. The templates are also files on the server that you can edit in a text editor.");
		b.Added("New permissions {yellow}MenuWebHooks:EditTemplates, TestSend, ViewURL{/}. Groups that could edit webhooks get them automatically. Without ViewURL, webhook addresses are hidden.");
		b.Added("Item Manager: double right-click an item to copy its class name to the clipboard. Hold {orange}Ctrl{/} to add it to a list instead, one name per line.");
		b.Changed("The Stats HUD remembers its corner and direction between sessions, and starts vertical.");
		b.Fixed("The {orange}K{/} repair key, /refuel and the Repair + replace missing parts action now work on boats and motorbikes.");
		b.Fixed("Notifications could get stuck on screen when many arrived at once.");
		b.Fixed("The admin free camera clashed with the new motorbike camera of DayZ 1.30.");
		b.Fixed("A crash when quitting the game or stopping the server after using the free camera.");
		b.Gap();
		b.Section("1.6", "2026-09-23");
		b.Added("{orange}Context Action Menu{/}: right-click players, items, weapons, vehicles and creatures to act on them directly.");
		b.Bullet("Look mode: with the admin tools on, hold {orange}RMB{/} on the object under your crosshair and click an action with {orange}LMB{/}. Release RMB to close. The mouse wheel or the arrow keys move through the rows, and Left goes back.");
		b.Bullet("Look mode is off by default. Press {orange}O{/} to turn it on or off. The choice is saved per client, and the key can be rebound in Controls under VPPAdminTools.");
		b.Bullet("While the look menu has the mouse, freecam movement and FOV zoom pause.");
		b.Bullet("Right-click a player in the Player Manager to open the same menu. With several players checked, the action applies to all of them.");
		b.Added("Player actions: heal, stop bleeding, spectate, in hands, copy Steam64 ID, clear inventory, kill, godmode, unlimited ammo, invisible, frozen, go to, bring to me, return to previous position, send message, kick and ban. They use the existing {yellow}PlayerManager{/} permissions.");
		b.Added("Weapon actions: jam and unjam, eject the chambered round, refill the magazine (it also chambers a round when the chamber is empty), and load a magazine or ammunition directly.");
		b.Added("Attachments and equipment: browse every slot of an item, weapon, vehicle or player, then spawn a compatible item into it, swap it or remove it. Weapons get separate Magazine and Ammunition pages.");
		b.Added("Item actions (repair to pristine, ruin, fill, empty), vehicle actions (repair, replace missing parts, refill all fluids), creature actions (heal, kill) and common actions (copy position, copy class name, delete).");
		b.Added("Dangerous actions such as {red}Delete{/}, {red}Kill{/}, {red}Ban{/} and {red}Ruin{/} need a second click. The row turns red and asks you to click again.");
		b.Added("New permissions: {yellow}ContextMenu:WeaponJam, WeaponEject, WeaponRefill, WeaponLoad, SpawnAttachment, ItemHealth, ItemQuantity, VehicleRefuel, CreatureHeal, CreatureKill{/}. Existing groups do not get them automatically, so grant them in the Permissions Editor.");
		b.Added("Modding API: other mods can add their own rows, submenus and permissions. The VPPContextMenuExample sample mod shows how.");
		b.Added("This changelog. It opens with the toolbar until you tick {muted}Don't show again{/}, and the next update brings it back. The {orange}changelog button{/} on the Stats HUD opens it any time.");
		b.Changed("{orange}Dynamic sidebar{/}: the toolbar grows by up to 3 rows for extra modules before it scrolls (2 at 1366x768, 3 at 1080p and taller). Thanks to DevDash-LM.");
		b.LinkRow("Dynamic sidebar pull request on GitHub", "https://github.com/VanillaPlusPlus/VPP-Admin-Tools/pull/212");
		b.Known("ESP Tools: showing and clearing combination-lock codes on base-building items is disabled until it is updated for DayZ 1.30.");
		b.Gap();
		b.LinkRow("Vanilla++ Discord", "https://discord.dayzvpp.com");
		b.LinkRow("VPP Admin Tools on GitHub", "https://github.com/VanillaPlusPlus/VPP-Admin-Tools");
	}
};
