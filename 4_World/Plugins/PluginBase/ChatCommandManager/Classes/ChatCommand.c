class ChatCommand
{
    private string m_Command;
    private int m_MinimumArgsCount;
    private string m_Permission;
    private bool m_PlayersAsArgs;
    
    void ChatCommand(string command, int argsCount, string permission, bool playerArgs = false)
    {
        m_Command = command;
        m_MinimumArgsCount = argsCount;
        m_Permission = permission;
        GetPermissionManager().AddPermissionType({permission});
        m_PlayersAsArgs = playerArgs;
    }
    
    bool HasPlayersAsArgs()
    {
        return m_PlayersAsArgs;
    }

    string GetCommand()
    {
        return m_Command;
    }
    
    int GetMinArgumentCount()
    {
        return m_MinimumArgsCount;
    }
    
    string GetPermissionName()
    {
        return m_Permission;
    }
}