class PresetItemData
{
	private string presetName;
	private ref array<ref Param2<string,bool>> itemsMap;
	
	void PresetItemData(string pName,string parent, array<string> childs)
	{
		presetName = pName;
		itemsMap   = new array<ref Param2<string,bool>>;
		Insert(parent,true);
		foreach(string child : childs)
		{
			Insert(child,false);
		}
	}
	
	private void Insert(string type, bool state)
	{
		itemsMap.Insert(new Param2<string,bool>(type,state));
	}
	
	string GetParentType(){
		foreach(Param2<string,bool> data: itemsMap){
			if (data.param2)
				return data.param1;
		}
		return "";
	}

	string GetPresetName(){
		return presetName;
	}
	
	array<string> GetItemTypes(){
		autoptr array<string> items = new array<string>;
		foreach(Param2<string,bool> data : itemsMap){
			items.Insert(data.param1);
		}
		return items;
	}
	
	void SetParentType(string newType){
		if (newType == "") return;
		
		for(int x = 0; x < itemsMap.Count(); x++ )
		{
			ref Param2<string,bool> data = itemsMap.Get(x);

			string itemType = data.param1;
			bool state      = data.param2;
			if (state){
				data.param1 = itemType;
				data.param2 = false;
			}

			if (itemType == newType){
				data.param1 = newType;
				data.param2 = true;
			}
		}
	}
	
	void SetPresetName(string name){
		presetName = name;
	}
	
	void AddItem(string item, bool parent){
		Insert(item,parent);
	}
	
	void RemoveItem(string item){
		for(int i = 0; i < itemsMap.Count(); i++)
		{
			autoptr Param2<string,bool> data = itemsMap.Get(i);
			if (data.param1 == item)
			{
				itemsMap.Remove(i);
				return;
			}
		}
	}
	
	bool IsParent(string item){
		foreach(Param2<string,bool> data : itemsMap)
		{
			if (data.param1 == item)
				return data.param2;
		}
		return false;
	}
}