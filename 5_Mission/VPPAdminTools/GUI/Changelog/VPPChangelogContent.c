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
