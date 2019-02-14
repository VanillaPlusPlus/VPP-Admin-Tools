//////////////////////////////////////////////
//Airdrop System 			   			 	//
//Author: GravityWolf 					 	//
//Github: github.com/gravitywolfnotamused	//
//////////////////////////////////////////////

typedef array<ref VPPSupply> Supplies;

class VPPSupply{
	private vector position;

	private Object package;
	private EntityAI packageAI;

	private bool m_HasGravity = false;
	private float currentLifeTime = 0;

	void VPPSupply(vector position, ref array<string> lootArray){
		this.position = position;

		package = GetGame().CreateObject("AirDrop_Container", position, false, true);
	 	package.SetDirection(package.GetDirection());
        package.SetPosition(Vector(package.GetPosition()[0],700,package.GetPosition()[2]));

		if(Class.CastTo(packageAI, package)){
			foreach(string ClassName : lootArray)
			{
				packageAI.GetInventory().CreateInInventory(ClassName);
			}
		}
		Print("Carepackage created at: " + position.ToString());
	}
	
	void ~VPPSupply(){}

	bool hasGravity()
	{
		return m_HasGravity;
	}

	void applyGravity(){
		Print("Doing the gravity shit");
		m_HasGravity = true;
		dBodySetLinearFactor(package, "0 -2 0");
		dBodyDestroy( package );
		autoptr PhysicsGeomDef geoms[] = { PhysicsGeomDef("", dGeomCreateBox( "2.5 2.5 6" ), "material/default", 0xffffffff )};
		dBodyCreateDynamicEx( package, dBodyGetCenterOfMass( package ), 500, geoms );
		position = package.GetPosition();
	}

	float getLifeTime(){
		return currentLifeTime;
	}
	void setCurrentLifetime(float Time){
		currentLifeTime += Time;
	}

	vector getPosition(){
		return position;
	}
	void setPosition(vector position){
		this.position = position;
		package.SetPosition(position);
	}

	Object getObject(){
		return package;
	}
	
	float getEndY(){
		return GetGame().SurfaceY(position[0], position[2]);
	}

	vector getEndYVector(){
		vector endVec = this.position;
		endVec[1] = getEndY();
		return endVec;
	}
}