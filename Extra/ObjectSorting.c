class VPPObjectSorting
{
	ScriptModule m_modulePtr;
	ref EspToolsMenu m_CallBackPtr;

	void VPPObjectSorting(ScriptModule module)
	{
		m_modulePtr = module;
	}
	
	void Destroy()
	{
		if (m_modulePtr)
			m_modulePtr.Release();
	}

	void SetCallBack(Class callback)
	{
		m_CallBackPtr = callback;
	}

	void Sort(map<Object, int> data)
	{
		map<typename,int> activeFilters = new map<typename,int>;
		foreach(VPPFilterEntry filter : m_CallBackPtr.m_FilterEntry)
		{
			if (!filter.IsSelected())
				continue;

			activeFilters.Insert(filter.GetFilterName().ToType(), filter.m_Props.color);
		}

		foreach(Object obj, int useless : data)
		{
			if (!obj)
				continue;
			if (!obj.HasNetworkID())
				continue;

			string objName = obj.GetDisplayName();
			if (m_CallBackPtr.m_ChkShowClassName.IsChecked() || objName == string.Empty)
				objName = obj.GetType();

			foreach(typename f, int color : activeFilters)
			{
				if (obj.IsInherited(f))
				{
					m_CallBackPtr.m_EspTrackers.Insert(obj, new VPPESPTracker(objName, obj, color));
				}
			}
		}
	}
};

ref VPPObjectSorting m_VOBSInstance;
VPPObjectSorting GetInstance(ScriptModule module)
{
	m_VOBSInstance = new VPPObjectSorting(module);
	return m_VOBSInstance;
};

void DestroyInstance()
{
	if (m_VOBSInstance)
	{
		m_VOBSInstance.Destroy();
		m_VOBSInstance = NULL;
	}
};