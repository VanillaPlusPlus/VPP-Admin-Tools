class VPPDebugMission: MissionGameplay
{
    PlayerBase m_Player

	void VPPDebugMission()
	{
	}

	override void OnInit()
	{
		super.OnInit();
		GetRPCManager().SendRPC("RPC_MissionGameplay", "AuthCheck", new Param1<bool>(true), true, NULL);//Pretend this is sent from server
		SpawnPlayer();
	}

	override void OnMissionStart()
	{
		super.OnMissionStart();
		Print("VPPDebugMission::OnMissionStart()");
	}

	override void OnMissionFinish()
	{
		CloseAllMenus();
		DestroyAllMenus();
		super.OnMissionFinish();
	}

	void _ToggleCam()
	{
		Sleep(1100);
		PluginDeveloper.GetInstance().ToggleFreeCamera();
		GetVPPUIManager().DisplayNotification("FreeCamera mode- > PRESS [HOME] to toggle", "V++ Admin Tools:", 8.0);
	}

    void SpawnPlayer()
    {
    	m_Player = GetGame().CreatePlayer(NULL, "SurvivorM_Boris", "1256.313 211.82 9349.465"/*GenerateSpawnPosition()*/, 0, "NONE");
    	EquipCharacter(GetGame().GetMenuDefaultCharacterData());
		GetGame().SelectPlayer(NULL, m_Player);
		m_Player.SetAllowDamage(false);

		thread _ToggleCam();
    }

    void EquipCharacter(MenuDefaultCharacterData char_data)
	{
		int slot_ID;
		string attachment_type;
		for (int i = 0; i < DefaultCharacterCreationMethods.GetAttachmentSlotsArray().Count(); i++)
		{
			slot_ID = DefaultCharacterCreationMethods.GetAttachmentSlotsArray().Get(i);
			attachment_type = "";
			if ( !char_data.GetAttachmentMap().Find(slot_ID,attachment_type) || !VerifyAttachmentType(slot_ID,attachment_type) )
			{
				//randomize
				if ( DefaultCharacterCreationMethods.GetConfigArrayCountFromSlotID(slot_ID) > 0 )
				{
					attachment_type = DefaultCharacterCreationMethods.GetConfigAttachmentTypes(slot_ID).GetRandomElement();
				}
				else //undefined, moving on
					continue;
			}
			
			if (attachment_type != "")
			{
				m_Player.GetInventory().CreateAttachmentEx(attachment_type,slot_ID);
			}
		}
	}

	bool VerifyAttachmentType(int slot_ID, string attachment_type)
	{
		return DefaultCharacterCreationMethods.GetConfigAttachmentTypes(slot_ID).Find(attachment_type) > -1;
	}

    vector GenerateSpawnPosition(vector worldCenter = vector.Zero, float surface_friction = 0.9, float safe_radius = 5.0)
	{
		vector position = vector.Zero;
		if (worldCenter == vector.Zero)
			worldCenter = GetGame().ConfigGetVector("CfgWorlds " + GetGame().GetWorldName() + " centerPosition");

		float world_width   = worldCenter[0] * 2;
		float world_height  = worldCenter[1] * 2;
		float x_min 		= worldCenter[0] / 2;
		float y_min 		= worldCenter[1] / 2;

		while(true)
		{
			float x = Math.RandomFloat(x_min, world_width);
			float z = Math.RandomFloat(y_min, world_height);
			position = Vector(x, GetGame().SurfaceY(x, z), z);
			if(!IsPosSafe(position, surface_friction, safe_radius))
				continue;

			break;
		}
		return position;
	}

	bool IsPosSafe(vector pos, float surface_friction = 0.9, float safe_radius = 5.0)
    {
		if(GetGame().SurfaceIsSea(pos[0], pos[2]) || GetGame().SurfaceIsPond(pos[0], pos[2]))
			return false;

		string surface_type;
		GetGame().SurfaceGetType(pos[0], pos[2], surface_type);

		string cfgSurfacePath = "CfgSurfaces " + surface_type;
		if(GetGame().ConfigGetInt(cfgSurfacePath + " interior") == 1)
			return false;

		float friction = GetGame().ConfigGetFloat(cfgSurfacePath + " friction");
		if (friction >= surface_friction)
		{
			vector start = pos;
			vector end = pos + vector.Up;
			PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
			Object m_HitObject;
			vector m_HitPosition;
			vector m_HitNormal;
			float  m_HitFraction;

			return DayZPhysics.SphereCastBullet(start, end, safe_radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction);
		}
		return false;
    }
};