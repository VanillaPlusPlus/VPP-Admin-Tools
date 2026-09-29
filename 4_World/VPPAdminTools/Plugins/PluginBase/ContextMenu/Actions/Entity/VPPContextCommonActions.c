class VPPCA_CommonCopyPosition : VPPContextAction
{
	override string GetId()
	{
		return "vpp.common.copy_position";
	}

	override string GetParentId()
	{
		return VPPContextConstants.GROUP_COPY;
	}

	override int GetOrder()
	{
		return 100;
	}

	override string GetPermission()
	{
		return "";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_COPY_POSITION";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_MAP_PIN;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target)
			return false;

		return target.HasObject();
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Object obj = target.GetObject();
		if (!obj)
			return true;

		//same format as MissionGameplay.CopyPositionClipboard
		string text = "Position: " + obj.GetPosition().ToString();
		text += "\nOrientation: " + obj.GetOrientation().ToString();
		text += "\nConfig-Type: " + obj.GetType();
		GetGame().CopyToClipboard(text);

		VPPContextActionManager.ClientNotify("#VSTR_CTX_RESULT_COPIED", VPPContextUtils.FormatPosition(obj.GetPosition()));
		return true;
	}
};

class VPPCA_CommonCopyType : VPPContextAction
{
	override string GetId()
	{
		return "vpp.common.copy_type";
	}

	override string GetParentId()
	{
		return VPPContextConstants.GROUP_COPY;
	}

	override int GetOrder()
	{
		return 110;
	}

	override string GetPermission()
	{
		return "";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_COPY_TYPE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_FILE_TEXT;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target)
			return false;

		return target.HasObject();
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Object obj = target.GetObject();
		if (!obj)
			return true;

		string typeName = obj.GetType();
		GetGame().CopyToClipboard(typeName);

		VPPContextActionManager.ClientNotify("#VSTR_CTX_RESULT_COPIED", typeName);
		return true;
	}
};

//Never offered for live players or vehicles with crew.
class VPPCA_CommonDelete : VPPContextAction
{
	override string GetId()
	{
		return "vpp.common.delete";
	}

	override int GetOrder()
	{
		return 9900;
	}

	override string GetPermission()
	{
		return "DeleteObjectAtCrosshair";
	}

	override bool AutoRegisterPermission()
	{
		return false;
	}

	override bool IsDanger(VPPContextTarget target)
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_DELETE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_TRASH;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer() || !target.HasObject())
			return false;

		return !VPPContextUtils.IsAliveMan(target.GetObject());
	}

	protected bool HasCrew(Object obj)
	{
		Transport t = Transport.Cast(obj);
		if (!t)
			return false;

		for (int i = 0; i < t.CrewSize(); i++)
		{
			if (t.CrewMember(i) != null)
				return true;
		}
		return false;
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		return !HasCrew(target.GetObject());
	}

	override bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		return !HasCrew(target.GetObject());
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Object obj = target.GetObject();
		if (!obj)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		if (VPPContextUtils.IsAliveMan(obj))
		{
			result.Fail("#VSTR_CTX_RESULT_INVALID");
			return;
		}

		string detail = VPPCA_EntityDetail.Describe(obj);

		EntityAI ent = EntityAI.Cast(obj);
		if (ent)
			ent.DeleteSafe();
		else
			GetGame().ObjectDelete(obj);

		result.Ok("#VSTR_CTX_RESULT_DELETED");
		result.LogDetail = detail;
	}
};
