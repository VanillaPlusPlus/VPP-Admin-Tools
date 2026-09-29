class VPPContextItem : Managed
{
	string Id;
	int    Kind;          //EVPPContextKind
	string Label;         //#KEY or literal
	string IconPath;          //full sprite path, empty = none
	string Hint;          //right-aligned secondary text (#KEY or literal)
	int    Order;
	bool   Enabled;       //default true
	bool   Checked;       //TOGGLE only
	bool   Danger;
	bool   Confirm;       //click-again confirmation
	bool   KeepOpen;      //stay open after execute
	ref VPPContextAction Action;    //STRONG; null for dynamic rows
	ref VPPContextTarget Target;    //ACTION/TOGGLE: execute on; SUBMENU: target of the page to open
	string  SubmenuId;              //SUBMENU: parentId of the page to open (empty = root page of Target)
	Managed CallbackInst;           //WEAK; must outlive the open menu (e.g. the provider itself)
	string  CallbackFunc;
	ref VPPContextArgs Args;        //optional per-row args, merged into the execution args (null = none)

	void VPPContextItem()
	{
		Kind    = EVPPContextKind.ACTION;
		Enabled = true;
		Order   = VPPContextConstants.ORDER_DEFAULT;
	}
};

class VPPContextItems : Managed
{
	protected ref array<ref VPPContextItem> m_Items;

	void VPPContextItems()
	{
		m_Items = new array<ref VPPContextItem>;
	}

	int Count()
	{
		return m_Items.Count();
	}

	VPPContextItem Get(int index)
	{
		if (index < 0 || index >= m_Items.Count())
			return null;
		return m_Items.Get(index);
	}

	VPPContextItem Find(string id)
	{
		int index = IndexOf(id);
		if (index < 0)
			return null;
		return m_Items.Get(index);
	}

	int IndexOf(string id)
	{
		for (int i = 0; i < m_Items.Count(); i++)
		{
			VPPContextItem item = m_Items.Get(i);
			if (item && item.Id == id)
				return i;
		}
		return -1;
	}

	void Insert(VPPContextItem item)
	{
		if (item)
			m_Items.Insert(item);
	}

	void InsertAt(VPPContextItem item, int index)
	{
		if (!item)
			return;

		int at = index;
		if (at < 0)
			at = 0;

		if (at >= m_Items.Count())
		{
			m_Items.Insert(item);
			return;
		}
		m_Items.InsertAt(item, at);
	}

	//appends and returns false when id is not found
	bool InsertBefore(string id, VPPContextItem item)
	{
		if (!item)
			return false;

		int index = IndexOf(id);
		if (index < 0)
		{
			m_Items.Insert(item);
			return false;
		}
		m_Items.InsertAt(item, index);
		return true;
	}

	bool Remove(string id)
	{
		int index = IndexOf(id);
		if (index < 0)
			return false;
		m_Items.RemoveOrdered(index);
		return true;
	}

	bool Replace(string id, VPPContextItem item)
	{
		if (!item)
			return false;

		int index = IndexOf(id);
		if (index < 0)
			return false;
		m_Items.Set(index, item);
		return true;
	}

	void Clear()
	{
		m_Items.Clear();
	}

	VPPContextItem AddAction(VPPContextAction action, VPPContextTarget target)
	{
		if (!action)
			return null;

		VPPContextItem item = new VPPContextItem();
		item.Id       = action.GetId();
		item.Kind     = action.GetKind();
		item.Label    = action.GetLabel(target);
		item.IconPath     = action.GetIcon(target);
		item.Hint     = action.GetHint(target);
		item.Order    = action.GetOrder();
		item.Enabled  = action.IsEnabled(target);
		item.Danger   = action.IsDanger(target);
		item.Confirm  = action.RequiresConfirm(target);
		item.KeepOpen = action.KeepMenuOpen(target);
		item.Action   = action;

		if (item.Kind == EVPPContextKind.TOGGLE)
			item.Checked = action.IsChecked(target);

		if (item.Kind == EVPPContextKind.SUBMENU)
		{
			item.Target    = action.GetSubmenuTarget(target);
			item.SubmenuId = action.GetSubmenuId(target);
		}
		else
		{
			item.Target = target;
		}

		m_Items.Insert(item);
		return item;
	}

	VPPContextItem AddSubmenu(string id, string label, string icon, int order, VPPContextTarget target, string submenuId)
	{
		VPPContextItem item = new VPPContextItem();
		item.Id        = id;
		item.Kind      = EVPPContextKind.SUBMENU;
		item.Label     = label;
		item.IconPath      = icon;
		item.Order     = order;
		item.Target    = target;
		item.SubmenuId = submenuId;
		m_Items.Insert(item);
		return item;
	}

	VPPContextItem AddCallback(string id, string label, string icon, int order, Managed inst, string funcName)
	{
		VPPContextItem item = new VPPContextItem();
		item.Id           = id;
		item.Kind         = EVPPContextKind.ACTION;
		item.Label        = label;
		item.IconPath         = icon;
		item.Order        = order;
		item.CallbackInst = inst;
		item.CallbackFunc = funcName;
		m_Items.Insert(item);
		return item;
	}

	VPPContextItem AddInfo(string id, string label, string icon, int order, string hint)
	{
		VPPContextItem item = new VPPContextItem();
		item.Id      = id;
		item.Kind    = EVPPContextKind.INFO;
		item.Label   = label;
		item.IconPath    = icon;
		item.Order   = order;
		item.Hint    = hint;
		item.Enabled = false;
		m_Items.Insert(item);
		return item;
	}

	VPPContextItem AddSeparator(int order)
	{
		VPPContextItem item = CreateSeparator(order, m_Items.Count());
		m_Items.Insert(item);
		return item;
	}

	//stable: Order asc (clamped to [0, ORDER_MAX] as an int), then Id asc, then insertion index
	void Sort()
	{
		int n = m_Items.Count();
		if (n < 2)
			return;

		map<string, ref VPPContextItem> byKey = new map<string, ref VPPContextItem>;
		array<string> keys = new array<string>;
		for (int i = 0; i < n; i++)
		{
			VPPContextItem item = m_Items.Get(i);
			if (!item)
				continue;

			int ord = item.Order;
			if (ord < 0)
				ord = 0;
			if (ord > VPPContextConstants.ORDER_MAX)
				ord = VPPContextConstants.ORDER_MAX;

			string key = ord.ToStringLen(6) + "|" + item.Id;
			key = key + "|" + i.ToStringLen(4);
			byKey.Set(key, item);
			keys.Insert(key);
		}

		keys.Sort();

		array<ref VPPContextItem> sorted = new array<ref VPPContextItem>;
		foreach (string sortedKey : keys)
		{
			sorted.Insert(byKey.Get(sortedKey));
		}
		m_Items = sorted;
	}

	//drops leading/trailing/duplicate separators and any directly after BACK,
	//then inserts one between neighbours whose (Order / 1000) bands differ
	void NormalizeSeparators()
	{
		array<ref VPPContextItem> cleaned = new array<ref VPPContextItem>;
		foreach (VPPContextItem it : m_Items)
		{
			if (!it)
				continue;

			if (it.Kind == EVPPContextKind.SEPARATOR)
			{
				if (cleaned.Count() == 0)
					continue;

				VPPContextItem prevClean = cleaned.Get(cleaned.Count() - 1);
				if (prevClean.Kind == EVPPContextKind.SEPARATOR || prevClean.Kind == EVPPContextKind.BACK)
					continue;
			}
			cleaned.Insert(it);
		}

		while (cleaned.Count() > 0)
		{
			VPPContextItem tail = cleaned.Get(cleaned.Count() - 1);
			if (tail.Kind != EVPPContextKind.SEPARATOR)
				break;
			cleaned.Remove(cleaned.Count() - 1);
		}

		array<ref VPPContextItem> banded = new array<ref VPPContextItem>;
		for (int i = 0; i < cleaned.Count(); i++)
		{
			VPPContextItem cur = cleaned.Get(i);
			if (banded.Count() > 0)
			{
				VPPContextItem prevOut = banded.Get(banded.Count() - 1);
				if (NeedsBandSeparator(prevOut, cur))
					banded.Insert(CreateSeparator(cur.Order, banded.Count()));
			}
			banded.Insert(cur);
		}

		m_Items = banded;
	}

	int CountExecutable()
	{
		int count = 0;
		foreach (VPPContextItem item : m_Items)
		{
			if (!item)
				continue;
			if (item.Kind == EVPPContextKind.ACTION || item.Kind == EVPPContextKind.TOGGLE || item.Kind == EVPPContextKind.SUBMENU)
				count++;
		}
		return count;
	}

	//-----------------------------------------------------------------
	// internals
	//-----------------------------------------------------------------
	protected VPPContextItem CreateSeparator(int order, int index)
	{
		string sepId = "vpp.ui.sep." + order.ToString();
		sepId = sepId + "." + index.ToString();

		VPPContextItem item = new VPPContextItem();
		item.Id      = sepId;
		item.Kind    = EVPPContextKind.SEPARATOR;
		item.Order   = order;
		item.Enabled = false;
		return item;
	}

	protected bool NeedsBandSeparator(VPPContextItem a, VPPContextItem b)
	{
		if (!a || !b)
			return false;
		if (a.Kind == EVPPContextKind.SEPARATOR || b.Kind == EVPPContextKind.SEPARATOR)
			return false;
		if (a.Kind == EVPPContextKind.BACK)
			return false;

		int bandA = a.Order / 1000;
		int bandB = b.Order / 1000;
		return bandA != bandB;
	}
};
