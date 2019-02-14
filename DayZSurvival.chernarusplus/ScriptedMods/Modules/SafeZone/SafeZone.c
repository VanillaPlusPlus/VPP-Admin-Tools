class SafeZone{
	private string name;
	private vector location;
	private float radius;

	void SafeZone(vector location, string name, float radius){
		this.location = location;
		this.name = name;
		this.radius = radius;
	}

	string getName(){
		return name;
	}

	float getRadius(){
		return radius;
	}

	vector getLocation(){
		return location;
	}
}