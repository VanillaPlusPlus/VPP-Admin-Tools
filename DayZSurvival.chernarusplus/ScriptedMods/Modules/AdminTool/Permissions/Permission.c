class Permission {
	private string name;
	private bool state;

	void Permission(string name, bool state){
		this.name = name;
		this.state = state;
	}

	string getName(){
		return name;
	}

	bool getState(){
		return state;
	}

	void setState(bool state){
		this.state = state;
	}
}
