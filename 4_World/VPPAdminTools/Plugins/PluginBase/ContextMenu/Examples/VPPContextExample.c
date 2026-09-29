#ifdef VPPAT_CONTEXT_EXAMPLES
/*
	VPP Context Action Menu - third-party integration example (never compiled; VPPAT_CONTEXT_EXAMPLES is never defined).
	A ready-to-build standalone example mod (client + server action) lives in P:\VPPContextMenuExample.
	Copy it into YOUR mod, rename the MyLocks_* classes and remove the outer VPPAT_CONTEXT_EXAMPLES guard.

	Packaging:
	- Guard every integration file with an ifdef on the VPPADMINTOOLS define so your mod still loads without VPP.
	- config.cpp: add DZM_VPPAdminTools to requiredAddons[] (or ship the integration as a separate PBO).
	- Integrations that add SERVER logic must live in 4_World (or lower) so client and server both register them.
	- Action ids use modtag.category.name (e.g. mylocks.vehicle.unlock). The vpp. prefix is reserved.

	Rules (frozen, contract section 4):
	A1 Actions are stateless singletons shared by every request. Never store per-target / per-request data in members.
	A2 Actions with server logic are registered from 4_World (or lower), so the client and server registries both know them.
	   The server looks actions up by id; an id unknown to the server gets INVALID.
	A3 A provider row that references an action should use a REGISTERED instance (GetVPPContextActionManager().GetAction(id)).
	   An unregistered instance still renders (Action is strong), but it can only run through OnExecuteClient returning true.
	A4 A row executes server-side only if that action's IsTargetType is true for the server-resolved target.
	A5 Never AddRPC on RPC_VPPContextMenu / RPC_VPPContextMenuClient (CF AddRPC is last-wins). Use your own namespace for async data.
	A6 VPPContextItem.CallbackInst is weak. It must outlive the open menu (use the provider itself, not a temporary).
	A7 Guard integration code with an ifdef on the VPPADMINTOOLS define, and add DZM_VPPAdminTools to requiredAddons.
	A8 New permissions are not granted to existing groups. Admins grant them in the Permissions Editor.
	A9 If AutoRegisterPermission() returns false, the mod registers the string itself (GetPermissionManager().AddPermissionType).

	Registration options:
	- modded class VPPContextActionManager { override RegisterActions() / RegisterProviders() } - ALWAYS call super first.
	- VPPContextActionManager.QueueAction(action) / QueueProvider(provider) from 4_World code: a PERSISTENT registration,
	  queued once per script load and re-applied to every manager instance (every mission), deduplicated by id.

	Changing built-ins:
	- ReplaceAction(builtinId, new MySubclass()) where MySubclass extends the built-in VPPCA_* class and GetId() is unchanged.
	- RemoveAction(builtinId) hides a built-in everywhere; a provider's items.Remove(builtinId) hides it for one target only.
	- modded class VPPCA_Xxx { override ... } from YOUR mod also works (VPP itself never mods its own VPPCA_* classes).

	Enforce reminders: no ternary operator, every call statement and method declaration on one physical line.
*/
#ifdef VPPADMINTOOLS
class MyLocks_CA_Unlock : VPPContextAction
{
	override string GetId()
	{
		return "mylocks.vehicle.unlock";
	}

	override string GetPermission()
	{
		return "MyLocks:AdminUnlock"; //auto-registered on the server (A8: not granted to any group)
	}

	override int GetOrder()
	{
		return 150; //after VPP refuel (120), same band -> no separator
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#STR_MYLOCKS_CTX_UNLOCK";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return "set:vpp_icons image:key_round";
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target)
			return false;

		if (target.IsPlayer())
			return false;

		if (!MyLocks_Car.Cast(target.GetObject()))
			return false;

		return true;
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		MyLocks_Car car = MyLocks_Car.Cast(target.GetObject());
		if (!car)
			return false;

		return car.MyLocks_IsLocked();
	}

	override bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		return IsEnabled(target);
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		MyLocks_Car car = MyLocks_Car.Cast(target.GetObject());
		if (!car)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		car.MyLocks_SetLocked(false);
		result.Ok("#STR_MYLOCKS_CTX_UNLOCKED");	//toast on the admin's client; VPP writes the log line + webhook
		result.LogDetail = car.GetType();
	}
};

//dynamic submenu: children are built client-side every time the page opens
class MyLocks_CA_Keys : VPPContextAction
{
	override string GetId()
	{
		return "mylocks.vehicle.keys";
	}

	override int GetKind()
	{
		return EVPPContextKind.SUBMENU;
	}

	override string GetPermission()
	{
		return ""; //container row, never executed server-side
	}

	override int GetOrder()
	{
		return 1500;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#STR_MYLOCKS_CTX_KEYS";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return "set:vpp_icons image:key_round";
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target)
			return false;

		if (target.IsPlayer())
			return false;

		if (!MyLocks_Car.Cast(target.GetObject()))
			return false;

		return true;
	}

	override void OnBuildChildren(VPPContextTarget target, VPPContextItems items)
	{
		MyLocks_Car car = MyLocks_Car.Cast(target.GetObject());
		if (!car)
			return;

		array<string> holders = car.MyLocks_GetKeyHolderNames();
		if (!holders)
			return;

		foreach (int i, string holderName : holders)
		{
			string rowId = "mylocks.vehicle.keys." + i.ToString();
			int rowOrder = 100 + i;
			items.AddInfo(rowId, holderName, "set:vpp_icons image:user", rowOrder, "");
		}
	}
};

//providers run client-side after the default pass and may add / remove / replace rows for one target
class MyLocks_ContextProvider : VPPContextProvider
{
	override void OnBuild(VPPContextTarget target, string parentId, VPPContextItems items)
	{
		if (!target)
			return;

		if (parentId != "")
			return;

		if (MyLocks_ElectricCar.Cast(target.GetObject()))
			items.Remove("vpp.vehicle.refuel");	//hide a VPP built-in for EVs only
	}
};

modded class VPPContextActionManager
{
	override void RegisterActions()
	{
		super.RegisterActions();
		AddAction(new MyLocks_CA_Unlock());
		AddAction(new MyLocks_CA_Keys());
	}

	override void RegisterProviders()
	{
		super.RegisterProviders();
		AddProvider(new MyLocks_ContextProvider());
	}
};

/*
	Server-only PBO variant: the client PBO ships MyLocks_CA_Unlock with the default OnExecuteServer (drop the override above),
	and a server-side-only PBO (loaded with -serverMod) adds the logic, keeping it out of the client download:

	modded class MyLocks_CA_Unlock
	{
		override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
		{
			MyLocks_Car car = MyLocks_Car.Cast(target.GetObject());
			if (!car)
			{
				result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
				return;
			}

			car.MyLocks_SetLocked(false);
			result.Ok("#STR_MYLOCKS_CTX_UNLOCKED");
		}
	};
*/

/*
	QueueAction alternative (instead of modding VPPContextActionManager), called from 4_World code.
	Queue BEFORE super.Init() so the entries are pending when the manager plugin is constructed:

	modded class PluginManager
	{
		override void Init()
		{
			VPPContextActionManager.QueueAction(new MyLocks_CA_Unlock());
			VPPContextActionManager.QueueAction(new MyLocks_CA_Keys());
			VPPContextActionManager.QueueProvider(new MyLocks_ContextProvider());
			super.Init();
		}
	};
*/
#endif
#endif
