/*
	VPP context action menu - public extension API (one VPPContextAction subclass per row).

	Third-party rules:
	A1  Actions are stateless singletons shared by every request. Never keep per-request state in members.
	A2  Actions with server logic are registered from 4_World (or lower), so the client and the server
	    registries both know them. The server looks actions up by id; an id unknown to the server is
	    answered with INVALID.
	A3  A provider row that references an action should use a REGISTERED instance
	    (GetVPPContextActionManager().GetAction(id)). An unregistered instance still renders (the row
	    holds a strong ref), but it can only run through OnExecuteClient returning true.
	A4  A row executes server-side only if that action's IsTargetType is true for the server-resolved target.
	A5  Never AddRPC on RPC_VPPContextMenu / RPC_VPPContextMenuClient (CF AddRPC is last-wins).
	    Use your own RPC namespace for async data.
	A6  VPPContextItem.CallbackInst is weak. It must outlive the open menu (e.g. use the provider itself).
	A7  Guard integration code with an ifdef on the VPPADMINTOOLS define and add DZM_VPPAdminTools to requiredAddons.
	A8  New permissions are not granted to existing user groups. Admins grant them in the Permissions Editor.
	A9  If AutoRegisterPermission() returns false, the mod registers the permission string itself
	    (GetPermissionManager().AddPermissionType), otherwise VerifyPermission rejects it.

	Registry changes: override RegisterActions / RegisterProviders in a modded VPPContextActionManager
	(call super first), or use the persistent VPPContextActionManager.QueueAction / QueueProvider.
	Change one built-in with a subclass plus ReplaceAction.
*/
class VPPContextAction : Managed
{
	//REQUIRED, lowercase <modtag>.<category>.<name>; the vpp. prefix is reserved
	string GetId()
	{
		return "";
	}

	//empty = root page, otherwise the id of a SUBMENU action
	string GetParentId()
	{
		return "";
	}

	int GetOrder()
	{
		return VPPContextConstants.ORDER_DEFAULT;
	}

	int GetKind()
	{
		return EVPPContextKind.ACTION;
	}

	//empty = client-only (never executed server-side)
	string GetPermission()
	{
		return "ContextMenu:" + GetId();
	}

	//true = the manager calls AddPermissionType server-side (rule A9 otherwise)
	bool AutoRegisterPermission()
	{
		return true;
	}

	//true = per-player-target and item-owner user-group level checks
	bool UsesTargetPermissionLevel()
	{
		return true;
	}

	string GetLabel(VPPContextTarget target)
	{
		return GetId();
	}

	string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_DOT;
	}

	string GetHint(VPPContextTarget target)
	{
		return "";
	}

	bool IsDanger(VPPContextTarget target)
	{
		return false;
	}

	bool IsChecked(VPPContextTarget target)
	{
		if (!target)
			return false;
		return target.GetStateBool(GetId(), false);
	}

	//runs on the client AND on the server (rule A4)
	bool IsTargetType(VPPContextTarget target)
	{
		return false;
	}

	//client: false hides the row
	bool CanShow(VPPContextTarget target)
	{
		return true;
	}

	//client: false greys the row out
	bool IsEnabled(VPPContextTarget target)
	{
		return true;
	}

	bool SupportsMultiTarget()
	{
		return false;
	}

	bool RequiresConfirm(VPPContextTarget target)
	{
		return IsDanger(target);
	}

	//non-empty = an input dialog opens first (title = GetLabel); the value arrives as args.GetInput()
	string GetInputPrompt(VPPContextTarget target)
	{
		return "";
	}

	bool IsInputNumeric()
	{
		return false;
	}

	bool KeepMenuOpen(VPPContextTarget target)
	{
		return GetKind() == EVPPContextKind.TOGGLE;
	}

	//same instance = drill into this action's children; another instance = retarget; null = hide the row
	VPPContextTarget GetSubmenuTarget(VPPContextTarget target)
	{
		return target;
	}

	//retargeting actions return an empty string (root page of the new target)
	string GetSubmenuId(VPPContextTarget target)
	{
		return GetId();
	}

	//SUBMENU dynamic children (client)
	void OnBuildChildren(VPPContextTarget target, VPPContextItems items)
	{
	}

	//true = handled locally; false = the manager sends ExecuteAction
	bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		return false;
	}

	bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		return true;
	}

	void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		if (result)
			result.Fail("#VSTR_CTX_RESULT_INVALID");
	}

	string GetLogMessage(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		string msg = GetId() + " -> ";
		if (target)
			msg = msg + target.Describe();
		return msg;
	}

	bool NeedsServerState(VPPContextTarget target)
	{
		return false;
	}

	//write under key GetId()
	void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
	}
};

class VPPContextProvider : Managed
{
	string GetId()
	{
		return ClassName();
	}

	//lower runs first
	int GetOrder()
	{
		return 0;
	}

	//client only; runs after the default pass for every page (parentId empty = root page)
	void OnBuild(VPPContextTarget target, string parentId, VPPContextItems items)
	{
	}
};
