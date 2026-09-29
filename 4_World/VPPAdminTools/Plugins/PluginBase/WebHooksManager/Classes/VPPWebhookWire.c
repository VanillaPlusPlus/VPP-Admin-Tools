// Server -> admin client replies of the webhook editor RPCs (RPC_MenuWebHooks).

class VPPWebhookIssueWire : Managed
{
	bool IsError;
	int Line;
	int Col;
	string Code;
	string Arg;
	string Message;
};

// ValidateTemplate / SaveTemplate: template problems, the preview rendered with sample values and the JSON verdict.
class VPPWebhookCheckReply : Managed
{
	int ReqId;
	string HookId;
	string EventId;
	bool Saved;
	bool TemplateOk;
	ref array<ref VPPWebhookIssueWire> Issues;
	string Preview;
	bool JsonChecked;
	bool JsonValid;
	string JsonMessage;
	int JsonLine;
	int JsonCol;
	ref array<string> ServiceProblems;

	void VPPWebhookCheckReply()
	{
		Issues = new array<ref VPPWebhookIssueWire>();
		ServiceProblems = new array<string>();
		Preview = "";
		JsonMessage = "";
	}
};

// GetTemplate: the template text of one event of one webhook.
class VPPWebhookTemplateReply : Managed
{
	int ReqId;
	string HookId;
	string EventId;
	string Text;
};

// TestSend: what the endpoint answered (DELIVERED, NO_REPLY or an error name).
class VPPWebhookTestReply : Managed
{
	int ReqId;
	string HookId;
	string EventId;
	bool Ok;
	string Reason;
};

// ReloadTemplates: per webhook, how many templates have problems (details are in the server log).
class VPPWebhookReloadReply : Managed
{
	int ReqId;
	ref array<string> HookIds;
	ref array<int> TemplateErrors;

	void VPPWebhookReloadReply()
	{
		HookIds = new array<string>();
		TemplateErrors = new array<int>();
	}
};
