//Reuses the existing RPC_AdminTools::RepairVehicles handler (it does its own permission check, logging and webhook).
class VPPCA_VehicleRepair : VPPContextAction
{
	override string GetId()
	{
		return "vpp.vehicle.repair";
	}

	override int GetOrder()
	{
		return 100;
	}

	override string GetPermission()
	{
		return "RepairVehiclesAtCrosshair";
	}

	override bool AutoRegisterPermission()
	{
		return false;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_VEH_REPAIR";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_HAMMER;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return Car.Cast(target.GetObject()) != null;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Object vehicle = target.GetObject();
		if (!vehicle)
			return true;

		GetRPCManager().VSendRPC("RPC_AdminTools", "RepairVehicles", new Param1<bool>(false), true, NULL, vehicle);
		return true;
	}
};

class VPPCA_VehicleRepairParts : VPPContextAction
{
	override string GetId()
	{
		return "vpp.vehicle.repair_parts";
	}

	override int GetOrder()
	{
		return 110;
	}

	override string GetPermission()
	{
		return "RepairVehiclesAtCrosshair";
	}

	override bool AutoRegisterPermission()
	{
		return false;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_VEH_REPAIR_PARTS";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_SETTINGS;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return Car.Cast(target.GetObject()) != null;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Object vehicle = target.GetObject();
		if (!vehicle)
			return true;

		GetRPCManager().VSendRPC("RPC_AdminTools", "RepairVehicles", new Param1<bool>(true), true, NULL, vehicle);
		return true;
	}
};

class VPPCA_VehicleRefuel : VPPContextAction
{
	override string GetId()
	{
		return "vpp.vehicle.refuel";
	}

	override int GetOrder()
	{
		return 120;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_VEHICLE_REFUEL;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_VEH_REFUEL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_DROPLETS;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return Transport.Cast(target.GetObject()) != null;
	}

	protected void FillFluidToCapacity(Transport t, ETransportFluid fluid)
	{
		//Intentional deviation from wp_P4 (which used GetFluidCapacityScript): that returns int and truncates the native float
		//capacity (e.g. 4.5 L oil -> 4), so read the native float per vehicle type, exactly as CarScript/BoatScript/MotorbikeScript
		//do internally (ETransportFluid values are passed straight to the per-type native; CarFluid/BoatFluid/MotorbikeFluid
		//derive from ETransportFluid). The Motorbike class ships in the same vanilla scripts as ETransportFluid/FillScript
		//(3_Game/dayz/Vehicles), which this file already requires, so it adds no extra version dependency.
		//Unknown Transport subclasses fall back to the int script API. Recorded as a P4 deviation (see decisions.md).
		float cap = 0;
		Car car = Car.Cast(t);
		Boat boat = Boat.Cast(t);
		Motorbike bike = Motorbike.Cast(t);
		if (car)
			cap = car.GetFluidCapacity(fluid);
		else if (boat)
			cap = boat.GetFluidCapacity(fluid);
		else if (bike)
			cap = bike.GetFluidCapacity(fluid);
		else
			cap = t.GetFluidCapacityScript(fluid);

		if (cap > 0)
			t.FillScript(fluid, cap);
	}

	//CarScript, BoatScript and MotorbikeScript implement the *Script fluid API (the Transport base is a no-op).
	//A driver-owned simulation may override these changes while the vehicle is being driven.
	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Transport t = Transport.Cast(target.GetObject());
		if (!t)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		FillFluidToCapacity(t, ETransportFluid.FUEL);
		FillFluidToCapacity(t, ETransportFluid.OIL);
		FillFluidToCapacity(t, ETransportFluid.BRAKE);
		FillFluidToCapacity(t, ETransportFluid.COOLANT);

		t.SetSynchDirty();
		t.Synchronize();

		result.Ok("#VSTR_CTX_RESULT_REFUELED");
		result.LogDetail = VPPCA_EntityDetail.Describe(t);
	}
};
