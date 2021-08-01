class RemoveBanChatModule extends ChatCommand
{
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();

		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /unban used. Input:  " + args));
		
		if (GetBansManager().RemoveMultipleFromBanList(args))
			GetPermissionManager().NotifyPlayer(callerID,"Successfully Removed ("+args.Count()+") bans from ban list!",NotifyTypes.NOTIFY);
		else
			GetPermissionManager().NotifyPlayer(callerID,"User(s) information is not in ban list!",NotifyTypes.ERROR);
	}
}