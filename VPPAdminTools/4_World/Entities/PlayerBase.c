modded class PlayerBase
{
	private bool hasGodmode;
	
	/*override void OnConnect()
    {
		super.OnConnect();
    }
	
	override void OnDisconnect()
	{
		super.OnDisconnect();
	}*/
	
	void setGodMode(bool trigger){
		hasGodmode = trigger;
		if (trigger)
		{
			this.SetAllowDamage(false);
		}else{
			this.SetAllowDamage(true);
		}
	}
	
	bool GodModeStatus(){
		return hasGodmode;
	}
}