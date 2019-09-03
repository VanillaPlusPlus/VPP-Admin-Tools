modded class PluginAdminLog
{	
	/* Send .adm logs over to our logging system. ( becuase we can't open .adm logs during runtime ) */
	override void LogPrint( string message )
	{
		super.LogPrint( message );
		GetSimpleLogger().Log(message);
	}
};