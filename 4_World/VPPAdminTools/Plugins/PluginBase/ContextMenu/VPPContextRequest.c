//wire format: plain serializable class, never carries an Object (sent with sendToTarget NULL)
class VPPContextRequest
{
	int    Seq;
	string ActionId;
	int    Kind;
	int    Source;
	int    NetLow;
	int    NetHigh;
	string TypeName;
	vector Position;
	string PlayerId;
	int    SessionId;
	string DisplayName;
	ref array<string> PlayerIds;
	ref array<int>    SessionIds;
	ref map<string,string> Extra;
	ref map<string,string> Args;

	void VPPContextRequest()
	{
		SessionId  = -1;
		PlayerIds  = new array<string>;
		SessionIds = new array<int>;
		Extra      = new map<string,string>;
		Args       = new map<string,string>;
	}
};

class VPPContextArgs : Managed
{
	protected ref map<string,string> m_Values;

	void VPPContextArgs()
	{
		m_Values = new map<string,string>;
	}

	static VPPContextArgs FromMap(map<string,string> values)
	{
		VPPContextArgs args = new VPPContextArgs();
		if (!values)
			return args;

		foreach (string key, string val : values)
		{
			args.m_Values.Set(key, val);
		}
		return args;
	}

	map<string,string> GetMap()
	{
		return m_Values;
	}

	bool Has(string key)
	{
		return m_Values.Contains(key);
	}

	string Get(string key, string fallback = "")
	{
		string val;
		if (m_Values.Find(key, val))
			return val;
		return fallback;
	}

	int GetInt(string key, int fallback = 0)
	{
		string val;
		if (!m_Values.Find(key, val))
			return fallback;
		return val.ToInt();
	}

	float GetFloat(string key, float fallback = 0.0)
	{
		string val;
		if (!m_Values.Find(key, val))
			return fallback;
		return val.ToFloat();
	}

	bool GetBool(string key, bool fallback = false)
	{
		string val;
		if (!m_Values.Find(key, val))
			return fallback;
		if (val == "1" || val == "true")
			return true;
		return false;
	}

	void Set(string key, string value)
	{
		m_Values.Set(key, value);
	}

	string GetInput()
	{
		return Get(VPPContextConstants.ARG_INPUT);
	}
};

class VPPContextResult : Managed
{
	bool   Success;      //default true
	string MessageKey;   //whole #VSTR key or literal; empty = no toast
	string MessageArg;   //appended after the translated key
	int    NewState;     //-1 n/a, 0 off, 1 on (toggles)
	bool   Silent;       //true = the reply carries an empty message key
	string LogDetail;    //appended to the central log line

	void VPPContextResult()
	{
		Success  = true;
		NewState = -1;
	}

	void Ok(string messageKey = "", string messageArg = "")
	{
		Success    = true;
		MessageKey = messageKey;
		MessageArg = messageArg;
	}

	void Fail(string messageKey, string messageArg = "")
	{
		Success    = false;
		MessageKey = messageKey;
		MessageArg = messageArg;
	}

	void SetState(bool state)
	{
		if (state)
			NewState = 1;
		else
			NewState = 0;
	}
};
