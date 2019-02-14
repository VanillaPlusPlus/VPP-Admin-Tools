//////////////////////////////////////////////
//Airdrop System 			   			 	//
//Author: GravityWolf 					 	//
//Github: github.com/gravitywolfnotamused	//
//////////////////////////////////////////////

#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\VPPSupply.c"

typedef array<ref VPPSupplyPlane> SupplyPlanes;

class VPPSupplyPlane{
	private vector position;
	private vector dropPosition;

	private EntityAI planeAI;
	private Object plane;
	private Land_Wreck_C130J m_PlaneClass;

	private bool hasDroppedSupplies;
	
	const float airPlaneSpeed = 0.015; //0.015
	const float planeHeight = 700;

	void VPPSupplyPlane(vector startingPosition, vector dropPosition){
		this.position = startingPosition;
		this.dropPosition = dropPosition;
		this.plane = GetGame().CreateObject("land_wreck_c130j", this.position, false, true);
		m_PlaneClass = Land_Wreck_C130J.Cast( plane );
		m_PlaneClass.setSoundStatus(true);
		
		Class.CastTo(planeAI, plane);
		plane.SetDirection(getDirection());
		hasDroppedSupplies = false;

		Print("Plane was spawn on the map at: " + position.ToString() + " headed for " + dropPosition.ToString());
	}

	void ~VPPSupplyPlane(){
		GetGame().ObjectDelete(plane);
	}
	
	void move(){
		float f = plane.GetOrientation()[0] * 0.017453292;
        float motionX  = (double)(Math.Sin(f) * airPlaneSpeed);
        float motionZ = (double)(Math.Cos(f) * airPlaneSpeed); 

		position[0] = position[0] - motionX;
		position[1] = planeHeight;
		position[2] = position[2] - motionZ;

		plane.SetPosition(position);
	}

	bool hasDroppedSupplies(){
		return hasDroppedSupplies;
	}

	void setHasDroppedSupplies(){
		hasDroppedSupplies = true;
	}
	
	vector getDirection(){
		return position - dropPosition;
	}

	Object getObject(){
		return plane;
	}

	vector getDropPosition(){
		return dropPosition;
	}

	vector getPosition(){
		return position;
	}

}