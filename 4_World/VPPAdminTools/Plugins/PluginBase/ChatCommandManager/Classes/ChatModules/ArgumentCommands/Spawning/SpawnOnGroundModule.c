class SpawnOnGroundChatModule : ChatCommand
{
    override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();

        foreach(string arg : args)
        {
            if(caller == null) return;

            EntityAI itemEntity;
            ItemBase itemBase;
            bool isAi = GetGame().IsKindOf( arg, "DZ_LightAI" );
            int iFlags = ECE_SETUP|ECE_KEEPHEIGHT|ECE_PLACE_ON_SURFACE;
        
            if ( isAi )
                iFlags = (ECE_CREATEPHYSICS * 2)|ECE_CREATEPHYSICS|iFlags;

            itemEntity = EntityAI.Cast( GetGame().CreateObjectEx( arg, caller.GetHeadingPosition(1), iFlags) );
    
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