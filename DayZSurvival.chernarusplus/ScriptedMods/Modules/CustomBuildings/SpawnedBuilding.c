class SpawnedBuilding
{
	string m_name;
	vector m_location;
	vector orientation;

	void SpawnedBuilding(string building_name, vector location, vector orientation)
	{
		this.m_name = building_name;
		this.m_location = location;
		this.orientation = orientation;
	}

	void ~SpawnedBuilding()
	{
		
	}

	string getName()
	{
		return this.m_name;
	}

	vector getLocation()
	{
		return this.m_location;
	}

	vector getOrientation()
	{
		return this.orientation;
	}
};