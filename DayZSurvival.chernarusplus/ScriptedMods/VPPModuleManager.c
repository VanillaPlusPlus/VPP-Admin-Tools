class VPPModuleManager
{
	ref VPPConfig m_VPPConfig = new VPPConfig();
	protected DayZSurvival m_MissionServer;

	void VPPModuleManager(DayZSurvival servermission)
	{
		this.m_MissionServer = servermission;
	}
	
	void ~VPPModuleManager()
	{
		
	}
	
	void Init()
	{
		
	}
	
	void onUpdate( float timeslice ) 
	{
		
	}
	
	void onMissionStart()
	{
		
	}
	
	void onMissionFinish()
	{
		
	}

	VPPConfig getConfig(){
		return m_VPPConfig;
	}

	string GetModuleName()
	{
		return ClassName();
	}

	typename GetModuleType()
	{
		return GetModuleName().ToType();
	}
}

//You might have to change my semicolons to {}//
class VPPConfig
{
	void VPPConfig();
	void ~VPPConfig();

	void createDefaults();

	void save(){
		Print("Saving " + ClassName());
	}
	
	void load(){
		Print("Loading " + ClassName());
	}

	string getSavePath();

	string getTypeName(){
		return ClassName();
	}

	typename getType(){
		return getTypeName().ToType();
	}
}

class VPPConfigManager extends VPPModuleManager
{
	ref array<typename> typeArray = new array<typename>;
	private ref array<ref VPPConfig> m_ModConfigs;
	void VPPConfigManager( DayZSurvival servermission, ref set<ref VPPModuleManager> modules)
	{
		Print("VPPConfigManager::()");
		m_ModConfigs = new array<ref VPPConfig>;
		loadConfigs(modules);
	}

	void ~VPPConfigManager()
	{

	}

	void Init()
	{
		Print("VPPConfigManager:: override void Init()");
	}

	/*Json Config Save class (use if plugin needs data to be saved/loaded)*/
	void saveConfig()
	{

	}

	ref array<ref VPPConfig> GetConfigArray()
	{
		return m_ModConfigs;
	}

    void loadConfigs(ref set<ref VPPModuleManager> modules)
    {
    	foreach(VPPModuleManager module : modules){	
			m_ModConfigs.Insert(VPPModuleManager.Cast(module).getConfig());
    	}
    	Print("Config count: " + m_ModConfigs.Count());

    	foreach(VPPConfig modconfigs : m_ModConfigs){
    		modconfigs.load();
    	}
    }
}