modded class CarScript
{
	void RefillAllLiquids()
	{
		float fuelReq = GetFluidCapacity(CarFluid.FUEL) - (GetFluidCapacity(CarFluid.FUEL) * GetFluidFraction(CarFluid.FUEL));
		float oilReq = GetFluidCapacity(CarFluid.OIL) - (GetFluidCapacity(CarFluid.OIL) * GetFluidFraction(CarFluid.OIL));
		float coolantReq = GetFluidCapacity(CarFluid.COOLANT) - (GetFluidCapacity(CarFluid.COOLANT) * GetFluidFraction(CarFluid.COOLANT));
		float brakeReq = GetFluidCapacity(CarFluid.BRAKE) - (GetFluidCapacity(CarFluid.BRAKE) * GetFluidFraction(CarFluid.BRAKE));
		Fill(CarFluid.FUEL, fuelReq);
		Fill(CarFluid.OIL, oilReq);
		Fill(CarFluid.COOLANT, coolantReq);
		Fill(CarFluid.BRAKE, brakeReq);
		SetSynchDirty();
		Synchronize();
	}
};