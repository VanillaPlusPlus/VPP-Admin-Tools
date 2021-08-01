/*
	Temporary, moving away from FileSerializer its causing issues when mods reach limit.
*/
class PermissionLoader
{
	void PermissionLoader()
	{
	}

	bool ConvertFormat(string filePath, inout array<Old_UserGroup> userGroups)
	{
		FileSerializer file = new FileSerializer();
		if (!file.Open(filePath, FileMode.READ))
			return false;

	    file.Read(userGroups);
	    file.Close();
	    GetSimpleLogger().Log("[PermissionLoader] Loaded UserGroups.vpp...Converting...");

	    string newPath = filePath;
	    newPath.Replace(".vpp", ".json");
	    JsonFileLoader<array<Old_UserGroup>>.JsonSaveFile(newPath, userGroups);
	    DeleteFile(filePath);
	    return true;
	}
};