class VPPEventHandler
{
	private ref map<string, ref ScriptInvoker> events;

	void VPPEventHandler()
	{
		events = new map<string, ref ScriptInvoker>;
		AddEvent("OnPlayerConnect");
		AddEvent("OnPlayerDisconnected");
		AddEvent("OnClientDisconnectCancel");
	}

	void ~VPPEventHandler()
	{
		foreach(string name, ScriptInvoker invoker : events)
		{
			if(invoker != null)
			{
				invoker.Clear();
			}
		}
		events.Clear();
		delete events;
	}

	void AddEvent(string eventName)
	{
		if(events.Contains(eventName))
		{
			Print("[" + ClassName() + "]:: Duplicate Event Added: " + eventName);
			return;
		}

		if(!events.Contains(eventName))
			events.Insert(eventName, new ScriptInvoker());
	}

	ScriptInvoker GetEventInvoker(string eventName)
	{
		ScriptInvoker eventInvoker;

		if(!events.Contains(eventName))
		{
			Print("[" + ClassName() + "]:: Invalid Event: " + eventName);
			return null;
		}

		if(events.Contains(eventName))
		{
			eventInvoker = events[eventName];
		}

		return eventInvoker;
	}
}