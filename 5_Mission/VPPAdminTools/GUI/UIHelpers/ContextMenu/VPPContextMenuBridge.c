/*
	Client-side spectate routing for the context menu.

	Installed by VPPContextMenuController through the public registry API
	(ReplaceAction), never as a modded VPPCA_* class. Mirrors MenuPlayerManager.SpectateTarget:
	the spectate client handler syncs the death-grace preference before entering.
	If the replace never happens, the 4_World RPC_PlayerManager fallback still works.
*/
class VPPContextSpectateAction : VPPCA_PlayerSpectate
{
	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		if (!target || target.GetPlayerId() == "")
			return true;

		if (GetVPPUIManager())
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_REQ");

		VPPSpectateClientHandler spectateClient = GetSpectateClient();
		if (spectateClient)
			spectateClient.RequestSpectate(target.GetPlayerId());

		return true;
	}
};
