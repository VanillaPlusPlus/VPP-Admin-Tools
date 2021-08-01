typedef Param4<int,UAIDWrapper,Class,string> VPPBindParams; //mode, wrapper ptr, callback class, callback method

enum VPPBinds
{
	Press   	= 2,
	Release 	= 4,
	Hold      	= 8,
	HoldBegin 	= 16,
	DoubleClick = 32,
	Click 	    = 64,
};

class VPPKeybindsManager: PluginBase
{
	protected static ref VPPKeybindsManager instance;
	protected ref map<int, ref VPPBindParams> m_KeyWrappers; //input ID, wrapper ptr
	protected ref ScriptInvoker m_KeyInvokers;

	void VPPKeybindsManager()
	{
		instance = this;
		m_KeyWrappers = new map<int, ref VPPBindParams>;
		m_KeyInvokers = new ScriptInvoker();
	}

	override void OnInit()
	{
	}

	override void OnUpdate(float delta_time)
	{
		if (m_KeyWrappers.Count() <= 0)
			return;

		foreach(int id, VPPBindParams params: m_KeyWrappers)
		{
			if (!params)
				continue;

			typename e = VPPBinds;
			for (int bit = 1, i = 0; i < e.GetVariableCount(); i++)
			{
				bit <<= 1;
				if (params.param1 & bit)
				{
					bool ret;
					g_Script.CallFunction(params.param2.InputP(), string.Format("Local%1", typename.EnumToString(VPPBinds, bit)), ret, NULL);
					if (ret){
						g_Script.CallFunction(params.param3, params.param4, NULL, NULL);
					}
				}
			}
		}
	}

	map<int, ref VPPBindParams> Binds()
	{
		return m_KeyWrappers;
	}

	/*
		For lazy calls, when key is not bound to callback function(s)
		Works with any input type, no register needed.
	*/
	static bool GetStatus(string inputName, VPPBinds mode)
	{
		UAInput input = GetUApi().GetInputByName(inputName);
		if (input == NULL)
			return false;

		bool ret;
		g_Script.CallFunction(input, string.Format("Local%1", typename.EnumToString(VPPBinds, mode)), ret, NULL);
		return ret;
	}

	/*
		Register bind to callback on function (global and or instance specific)
		@Param0: inputName, name of UA input defined with input.xml
		@Param1: mode, type of bind behavior, check VPPBinds enum
		@Param2: callBackFunc, method that will be called on bind event
		@Param3: endPoint, instance of desired callback method, if NULL function will be called globally
	*/
	static void RegisterBind(string inputName, VPPBinds mode, string callBackFunc, Class endPoint = NULL)
	{
		UAInput input = GetUApi().GetInputByName(inputName);
		if (input == NULL)
		{
			Print(string.Format("[VPPKeybindsManager] RegisterBind() :: Failed to bind [%1] is an invalid input! check spelling on input name.", inputName));
			return;
		}

		VPPBindParams params = new VPPBindParams(mode, input.GetPersistentWrapper(), endPoint, callBackFunc);
		VPPKeybindsManager.GetInstance().Binds().Insert(input.ID(), params);
		Print(string.Format("[VPPKeybindsManager] RegisterBind() :: Registered bind: [%1] mode: [%2] callback function: [%3]", inputName, typename.EnumToString(VPPBinds, mode), callBackFunc));
	}

	static VPPKeybindsManager GetInstance()
	{
		return instance;
	}
};