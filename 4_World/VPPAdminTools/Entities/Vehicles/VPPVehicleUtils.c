/*
	Vehicle helpers shared by the repair keybind (K), the context menu repair actions and /refuel: they work on every
	Transport the game has (cars, boats, motorbikes), not only on Car.
*/
class VPPVehicleUtils
{
	//The native float capacity per vehicle type: the *Script getter returns an int and truncates (4.5 L oil -> 4).
	//ETransportFluid values are passed straight to the per-type native (CarFluid / BoatFluid / MotorbikeFluid).
	static float FluidCapacity(Transport vehicle, ETransportFluid fluid)
	{
		Car car = Car.Cast(vehicle);
		if (car)
		{
			return car.GetFluidCapacity(fluid);
		}

		Boat boat = Boat.Cast(vehicle);
		if (boat)
		{
			return boat.GetFluidCapacity(fluid);
		}

		Motorbike bike = Motorbike.Cast(vehicle);
		if (bike)
		{
			return bike.GetFluidCapacity(fluid);
		}

		return vehicle.GetFluidCapacityScript(fluid);
	}

	//Tops every fluid the vehicle has up to its capacity (a fluid it does not have reports capacity 0 and is skipped).
	static void RefillAllFluids(Transport vehicle)
	{
		if (!vehicle)
		{
			return;
		}

		TopUpFluid(vehicle, ETransportFluid.FUEL);
		TopUpFluid(vehicle, ETransportFluid.OIL);
		TopUpFluid(vehicle, ETransportFluid.BRAKE);
		TopUpFluid(vehicle, ETransportFluid.COOLANT);
		vehicle.SetSynchDirty();
		vehicle.Synchronize();
	}

	protected static void TopUpFluid(Transport vehicle, ETransportFluid fluid)
	{
		float capacity = FluidCapacity(vehicle, fluid);
		if (capacity <= 0)
		{
			return;
		}

		float fraction = vehicle.GetFluidFractionScript(fluid);
		float missing = capacity - capacity * fraction;
		if (missing > 0)
		{
			vehicle.FillScript(fluid, missing);
		}
	}

	//A part for an empty attachment slot ("" when the game has no item for it): a non-ruined variant when there is one.
	static string PartForSlot(string slotLower)
	{
		array<string> candidates = VPPATInventorySlots.SlotsItems.Get(slotLower);
		if (!candidates || candidates.Count() == 0)
		{
			return "";
		}

		string typeName = candidates.GetRandomElement();
		string lowerName = typeName;
		lowerName.ToLower();
		if (!lowerName.Contains("_ruined"))
		{
			return typeName;
		}

		foreach (string candidate : candidates)
		{
			string candidateLower = candidate;
			candidateLower.ToLower();
			if (!candidateLower.Contains("_ruined"))
			{
				return candidate;
			}
		}

		return "";
	}
};
