class SpawnOnGroundChatModule : ChatCommand
{
    override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        PlayerIdentity identity = caller.GetIdentity();

        if(identity == null) return;

        string callerID;
        string callerName;

        if(identity != null)
        {
            callerID = identity.GetPlainId();
            callerName = identity.GetName();
        }

        foreach(string arg : args)
        {
            if(caller == null) return;

            EntityAI itemEntity;
            ItemBase itemBase;
            bool isAi = GetGame().IsKindOf( arg, "DZ_LightAI" );
            itemEntity = EntityAI.Cast( GetGame().CreateObject( arg, caller.GetHeadingPosition(1), false, isAi ) );
    
            if ( itemEntity == NULL )
                return;
        
            itemEntity.SetHealth( itemEntity.GetMaxHealth() );
            
            if ( itemEntity.IsInherited( ItemBase ) && !isAi )
            {
                itemBase = ItemBase.Cast( itemEntity );
                itemBase.SetupSpawnedItem( itemBase, itemBase.GetMaxHealth("",""), itemBase.GetQuantityMax() );
            }

            itemEntity.PlaceOnSurface();
            GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /spg used on self. Input (%3)", callerName, callerID, arg));
            GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /spg used on self. Input:  " + arg));
        }
    }
}