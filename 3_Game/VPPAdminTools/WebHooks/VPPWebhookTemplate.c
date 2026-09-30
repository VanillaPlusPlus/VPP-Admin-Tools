/*
	Webhook body templates: the request body as text with placeholders.

		{{victim.name}}                    a variable (JSON-escaped when the webhook sends JSON)
		{{victim.id|steamurl}}             filters, applied left to right
		{{weapon|default:"unknown"|upper}} filter arguments may be quoted
		{{#if killer.id}}...{{else}}...{{/if}}      shown when the variable is not empty
		{{#unless killer.id}}...{{/unless}}          shown when it is empty
		{{#ifeq cause "suicide"}}...{{/ifeq}}         shown when it equals the value
		{{! a comment }}

	Filters: upper, lower, trim, truncate:N, default:TEXT, round:N, number, steamurl, raw (no escaping: insert JSON),
	json (a complete JSON string literal, quotes included).

	Parse once, render many times. Problems carry line / column and a stable code (the admin UI localizes them).
*/
class VPPWebhookIssue : Managed
{
	bool IsError;
	int Line;
	int Col;
	// stable id of the problem (UI text key suffix), e.g. "UNKNOWN_VAR"
	string Code;
	string Arg;
	// English text for the server log
	string Message;
};

class VPPWebhookNode : Managed
{
	const static int TEXT = 0;
	const static int VAR = 1;
	const static int COND = 2;
	const static int TEST_IF = 0;
	const static int TEST_UNLESS = 1;
	const static int TEST_IFEQ = 2;

	int Kind;
	int CondTest;
	int Line;
	int Col;
	string Text;
	string Name;
	string Value;
	ref array<string> Filters;
	ref array<string> FilterArgs;
	ref array<ref VPPWebhookNode> Kids;
	ref array<ref VPPWebhookNode> ElseKids;
	bool InElse;

	void VPPWebhookNode(int kind)
	{
		Kind = kind;
		Text = "";
		Name = "";
		Value = "";
		Filters = new array<string>();
		FilterArgs = new array<string>();
		Kids = new array<ref VPPWebhookNode>();
		ElseKids = new array<ref VPPWebhookNode>();
	}
};

class VPPWebhookTemplate : Managed
{
	// engine Substring returns at most 8191 bytes: a template stays below that
	const static int MAX_LENGTH = 8000;

	protected ref array<ref VPPWebhookNode> m_Root;
	protected string m_Source;
	ref array<ref VPPWebhookIssue> Issues;

	void VPPWebhookTemplate()
	{
		m_Root = new array<ref VPPWebhookNode>();
		Issues = new array<ref VPPWebhookIssue>();
		m_Source = "";
	}

	bool HasErrors()
	{
		foreach (VPPWebhookIssue issue : Issues)
		{
			if (issue.IsError)
			{
				return true;
			}
		}

		return false;
	}

	// def: the event the template is for (null = custom event, variable names are not checked).
	bool Parse(string source, VPPWebhookEventDef def)
	{
		m_Source = source;
		m_Root.Clear();
		Issues.Clear();
		if (source.Length() > MAX_LENGTH)
		{
			string maxText = MAX_LENGTH.ToString();
			string tooLong = "The template is longer than " + maxText + " characters.";
			AddIssue(true, 0, "TOO_LONG", maxText, tooLong);
			return false;
		}

		array<VPPWebhookNode> openConds = new array<VPPWebhookNode>();
		int pos = 0;
		int total = source.Length();
		while (pos < total)
		{
			int open = source.IndexOfFrom(pos, "{{");
			if (open < 0)
			{
				string tail = source.Substring(pos, total - pos);
				AddText(openConds, tail);
				break;
			}

			if (open > pos)
			{
				string before = source.Substring(pos, open - pos);
				AddText(openConds, before);
			}

			int close = source.IndexOfFrom(open + 2, "}}");
			if (close < 0)
			{
				AddIssue(true, open, "UNCLOSED_TAG", "", "A {{ tag is never closed with }}.");
				break;
			}

			string inner = source.Substring(open + 2, close - open - 2);
			ParseTag(inner, open, def, openConds);
			pos = close + 2;
		}

		foreach (VPPWebhookNode stillOpen : openConds)
		{
			string unclosed = "A {{#...}} block for " + stillOpen.Name + " is never closed.";
			AddIssueAt(true, stillOpen.Line, stillOpen.Col, "UNCLOSED_BLOCK", stillOpen.Name, unclosed);
		}

		return !HasErrors();
	}

	protected void ParseTag(string inner, int tagPos, VPPWebhookEventDef def, array<VPPWebhookNode> openConds)
	{
		string tag = inner.Trim();
		if (tag == "")
		{
			AddIssue(true, tagPos, "EMPTY_TAG", "", "Empty {{ }} tag.");
			return;
		}

		string first = tag.Substring(0, 1);
		if (first == "!")
		{
			return;
		}

		if (first == "#")
		{
			ParseOpen(tag, tagPos, def, openConds);
			return;
		}

		if (first == "/")
		{
			ParseClose(tag, tagPos, openConds);
			return;
		}

		if (tag == "else")
		{
			int depth = openConds.Count();
			if (depth == 0)
			{
				AddIssue(true, tagPos, "STRAY_ELSE", "", "{{else}} outside of an {{#if}} block.");
				return;
			}

			VPPWebhookNode top = openConds[depth - 1];
			if (top.InElse)
			{
				AddIssue(true, tagPos, "DOUBLE_ELSE", top.Name, "A second {{else}} in the same block.");
				return;
			}

			top.InElse = true;
			return;
		}

		ParseVar(tag, tagPos, def, openConds);
	}

	protected void ParseOpen(string tag, int tagPos, VPPWebhookEventDef def, array<VPPWebhookNode> openConds)
	{
		int space = tag.IndexOf(" ");
		string word = tag;
		string rest = "";
		if (space > 0)
		{
			word = tag.Substring(0, space);
			rest = tag.Substring(space + 1, tag.Length() - space - 1);
			rest = rest.Trim();
		}

		VPPWebhookNode cond = new VPPWebhookNode(VPPWebhookNode.COND);
		SetPos(cond, tagPos);
		if (word == "#if")
		{
			cond.CondTest = VPPWebhookNode.TEST_IF;
		}
		else if (word == "#unless")
		{
			cond.CondTest = VPPWebhookNode.TEST_UNLESS;
		}
		else if (word == "#ifeq")
		{
			cond.CondTest = VPPWebhookNode.TEST_IFEQ;
		}
		else
		{
			string unknownBlock = "Unknown block " + word + " (use #if, #unless or #ifeq).";
			AddIssue(true, tagPos, "UNKNOWN_BLOCK", word, unknownBlock);
			return;
		}

		string varName = rest;
		if (cond.CondTest == VPPWebhookNode.TEST_IFEQ)
		{
			int valueStart = rest.IndexOf(" ");
			if (valueStart < 0)
			{
				AddIssue(true, tagPos, "IFEQ_VALUE", rest, "{{#ifeq}} needs a variable and a value.");
				return;
			}

			varName = rest.Substring(0, valueStart);
			string compared = rest.Substring(valueStart + 1, rest.Length() - valueStart - 1);
			compared = compared.Trim();
			cond.Value = Unquote(compared);
		}

		if (varName == "")
		{
			string blockVar = word + " needs a variable name.";
			AddIssue(true, tagPos, "BLOCK_VAR", word, blockVar);
			return;
		}

		cond.Name = varName;
		CheckVarName(varName, tagPos, def);
		AppendNode(openConds, cond);
		openConds.Insert(cond);
	}

	protected void ParseClose(string tag, int tagPos, array<VPPWebhookNode> openConds)
	{
		int depth = openConds.Count();
		if (depth == 0)
		{
			string strayClose = tag + " has no matching opening block.";
			AddIssue(true, tagPos, "STRAY_CLOSE", tag, strayClose);
			return;
		}

		VPPWebhookNode top = openConds[depth - 1];
		string expected = "/if";
		if (top.CondTest == VPPWebhookNode.TEST_UNLESS)
		{
			expected = "/unless";
		}
		else if (top.CondTest == VPPWebhookNode.TEST_IFEQ)
		{
			expected = "/ifeq";
		}

		if (tag != expected)
		{
			string wrongClose = tag + " closes a block that needs {{" + expected + "}}.";
			AddIssue(true, tagPos, "WRONG_CLOSE", expected, wrongClose);
		}

		openConds.Remove(depth - 1);
	}

	protected void ParseVar(string tag, int tagPos, VPPWebhookEventDef def, array<VPPWebhookNode> openConds)
	{
		array<string> parts = new array<string>();
		SplitFilters(tag, parts);
		VPPWebhookNode varNode = new VPPWebhookNode(VPPWebhookNode.VAR);
		SetPos(varNode, tagPos);
		string varName = parts[0];
		varName = varName.Trim();
		varNode.Name = varName;
		CheckVarName(varName, tagPos, def);
		for (int i = 1; i < parts.Count(); i++)
		{
			string filterText = parts[i];
			filterText = filterText.Trim();
			string filterName = filterText;
			string filterArg = "";
			int colon = filterText.IndexOf(":");
			if (colon > 0)
			{
				filterName = filterText.Substring(0, colon);
				string rawArg = filterText.Substring(colon + 1, filterText.Length() - colon - 1);
				filterArg = Unquote(rawArg.Trim());
			}

			if (!CheckFilter(filterName, filterArg, tagPos))
			{
				continue;
			}

			varNode.Filters.Insert(filterName);
			varNode.FilterArgs.Insert(filterArg);
		}

		AppendNode(openConds, varNode);
	}

	// "a|b:\"x|y\"" -> a, b:"x|y" (a | inside quotes stays in the argument)
	protected void SplitFilters(string tag, array<string> outParts)
	{
		string current = "";
		bool quoted = false;
		int total = tag.Length();
		for (int i = 0; i < total; i++)
		{
			string ch = tag.Get(i);
			if (ch == "\"")
			{
				quoted = !quoted;
			}

			if (ch == "|" && !quoted)
			{
				outParts.Insert(current);
				current = "";
				continue;
			}

			current = current + ch;
		}

		outParts.Insert(current);
	}

	protected bool CheckFilter(string filterName, string filterArg, int tagPos)
	{
		if (filterName == "upper" || filterName == "lower" || filterName == "trim" || filterName == "steamurl")
		{
			return true;
		}

		if (filterName == "raw" || filterName == "json" || filterName == "number" || filterName == "default")
		{
			return true;
		}

		if (filterName == "truncate" || filterName == "round")
		{
			int argNumber = filterArg.ToInt();
			string argBack = argNumber.ToString();
			if (filterArg == "" || argBack != filterArg || argNumber < 0)
			{
				string argMessage = "The " + filterName + " filter needs a whole number, e.g. " + filterName + ":2.";
				AddIssue(true, tagPos, "FILTER_ARG", filterName, argMessage);
				return false;
			}

			return true;
		}

		string unknownFilter = "Unknown filter " + filterName + ".";
		AddIssue(true, tagPos, "UNKNOWN_FILTER", filterName, unknownFilter);
		return false;
	}

	protected void CheckVarName(string varName, int tagPos, VPPWebhookEventDef def)
	{
		if (!IsPlainVarName(varName))
		{
			string badVar = varName + " is not a valid variable name (letters, digits, dots, _).";
			AddIssue(true, tagPos, "BAD_VAR", varName, badVar);
			return;
		}

		if (!def || VPPWebhookDefs.IsHookVar(varName) || def.HasVar(varName))
		{
			return;
		}

		string unknownVar = "The " + def.Id + " event has no variable " + varName + ".";
		AddIssue(true, tagPos, "UNKNOWN_VAR", varName, unknownVar);
	}

	protected bool IsPlainVarName(string varName)
	{
		int total = varName.Length();
		if (total == 0)
		{
			return false;
		}

		for (int i = 0; i < total; i++)
		{
			string ch = varName.Get(i);
			int code = ch.ToAscii();
			bool lower = code >= 97 && code <= 122;
			bool upper = code >= 65 && code <= 90;
			bool digit = code >= 48 && code <= 57;
			if (!lower && !upper && !digit && ch != "." && ch != "_")
			{
				return false;
			}
		}

		return true;
	}

	protected void AddText(array<VPPWebhookNode> openConds, string text)
	{
		if (text == "")
		{
			return;
		}

		VPPWebhookNode textNode = new VPPWebhookNode(VPPWebhookNode.TEXT);
		textNode.Text = text;
		AppendNode(openConds, textNode);
	}

	protected void AppendNode(array<VPPWebhookNode> openConds, VPPWebhookNode child)
	{
		int depth = openConds.Count();
		if (depth == 0)
		{
			m_Root.Insert(child);
			return;
		}

		VPPWebhookNode top = openConds[depth - 1];
		if (top.InElse)
		{
			top.ElseKids.Insert(child);
		}
		else
		{
			top.Kids.Insert(child);
		}
	}

	// ---------------------------------------------------------------- render

	// vars: event + hook variables. jsonEscape: the body is JSON (values are escaped for a JSON string).
	const static int BRANCH_ROOT = 0;
	const static int BRANCH_KIDS = 1;
	const static int BRANCH_ELSE = 2;

	// A flat walk with an explicit stack: recursion is unreliable in Enforce (proven here twice, an inner call
	// clobbered the outer frame: first a null node, then the outer loops stopped and the rest of the body was lost).
	// A stack entry is the node whose children are walked (null = the root), which child list and the next index.
	string Render(map<string, string> vars, bool jsonEscape)
	{
		string output = "";
		string current = "";
		string rendered = "";
		bool passed = false;
		int depth = 0;
		int branch = 0;
		int index = 0;
		int count = 0;
		VPPWebhookNode parentNode = null;
		VPPWebhookNode tplNode = null;
		array<VPPWebhookNode> stackNodes = new array<VPPWebhookNode>();
		array<int> stackBranches = new array<int>();
		array<int> stackIndexes = new array<int>();
		stackNodes.Insert(null);
		stackBranches.Insert(BRANCH_ROOT);
		stackIndexes.Insert(0);
		while (stackIndexes.Count() > 0)
		{
			depth = stackIndexes.Count() - 1;
			parentNode = stackNodes[depth];
			branch = stackBranches[depth];
			index = stackIndexes[depth];
			count = ChildCount(parentNode, branch);
			if (index >= count)
			{
				stackNodes.RemoveOrdered(depth);
				stackBranches.RemoveOrdered(depth);
				stackIndexes.RemoveOrdered(depth);
				continue;
			}

			stackIndexes.Set(depth, index + 1);
			tplNode = ChildAt(parentNode, branch, index);
			if (!tplNode)
			{
				continue;
			}

			if (tplNode.Kind == VPPWebhookNode.TEXT)
			{
				output = output + tplNode.Text;
				continue;
			}

			current = "";
			if (vars.Contains(tplNode.Name))
			{
				current = vars.Get(tplNode.Name);
			}

			if (tplNode.Kind == VPPWebhookNode.VAR)
			{
				rendered = RenderVar(tplNode, current, jsonEscape);
				output = output + rendered;
				continue;
			}

			passed = current != "";
			if (tplNode.CondTest == VPPWebhookNode.TEST_UNLESS)
			{
				passed = current == "";
			}
			else if (tplNode.CondTest == VPPWebhookNode.TEST_IFEQ)
			{
				passed = current == tplNode.Value;
			}

			stackNodes.Insert(tplNode);
			if (passed)
			{
				stackBranches.Insert(BRANCH_KIDS);
			}
			else
			{
				stackBranches.Insert(BRANCH_ELSE);
			}

			stackIndexes.Insert(0);
		}

		return output;
	}

	protected int ChildCount(VPPWebhookNode parentNode, int branch)
	{
		if (branch == BRANCH_ROOT)
		{
			return m_Root.Count();
		}

		if (!parentNode)
		{
			return 0;
		}

		if (branch == BRANCH_KIDS)
		{
			return parentNode.Kids.Count();
		}

		return parentNode.ElseKids.Count();
	}

	protected VPPWebhookNode ChildAt(VPPWebhookNode parentNode, int branch, int index)
	{
		if (branch == BRANCH_ROOT)
		{
			return m_Root[index];
		}

		if (!parentNode)
		{
			return null;
		}

		if (branch == BRANCH_KIDS)
		{
			return parentNode.Kids[index];
		}

		return parentNode.ElseKids[index];
	}

	protected string RenderVar(VPPWebhookNode varNode, string rawValue, bool jsonEscape)
	{
		string value = rawValue;
		string filterName = "";
		string filterArg = "";
		string escapedValue = "";
		bool escape = jsonEscape;
		int total = varNode.Filters.Count();
		int i = 0;
		for (i = 0; i < total; i++)
		{
			filterName = varNode.Filters[i];
			filterArg = varNode.FilterArgs[i];
			if (filterName == "raw")
			{
				escape = false;
			}
			else if (filterName == "json")
			{
				escapedValue = EscapeJson(value);
				value = "\"" + escapedValue + "\"";
				escape = false;
			}
			else
			{
				value = ApplyFilter(filterName, filterArg, value);
			}
		}

		if (escape)
		{
			escapedValue = EscapeJson(value);
			return escapedValue;
		}

		return value;
	}

	protected string ApplyFilter(string filterName, string filterArg, string rawValue)
	{
		string value = rawValue;
		string result = "";
		int maxChars = 0;
		int decimals = 0;
		float numeric = 0;
		if (filterName == "upper")
		{
			value.ToUpper();
			return value;
		}

		if (filterName == "lower")
		{
			value.ToLower();
			return value;
		}

		if (filterName == "trim")
		{
			result = value.Trim();
			return result;
		}

		if (filterName == "default")
		{
			if (value == "")
			{
				return filterArg;
			}

			return value;
		}

		if (filterName == "steamurl")
		{
			if (value == "")
			{
				return "";
			}

			result = "https://steamcommunity.com/profiles/" + value;
			return result;
		}

		if (filterName == "truncate")
		{
			maxChars = filterArg.ToInt();
			if (value.LengthUtf8() <= maxChars || maxChars < 1)
			{
				return value;
			}

			result = value.SubstringUtf8(0, maxChars - 1);
			result = result + "…";
			return result;
		}

		if (filterName == "number")
		{
			if (IsNumberText(value))
			{
				return value;
			}

			return "0";
		}

		if (filterName == "round")
		{
			if (!IsNumberText(value))
			{
				return value;
			}

			numeric = value.ToFloat();
			decimals = filterArg.ToInt();
			result = RoundText(numeric, decimals);
			return result;
		}

		return value;
	}

	// ---------------------------------------------------------------- helpers

	static string EscapeJson(string value)
	{
		string escaped = value;
		escaped.Replace("\\", "\\\\");
		escaped.Replace("\"", "\\\"");
		escaped.Replace("\n", "\\n");
		int carriageCode = 13;
		string carriage = carriageCode.AsciiToString();
		escaped.Replace(carriage, "\\r");
		escaped.Replace("\t", "\\t");
		return escaped;
	}

	static bool IsNumberText(string text)
	{
		int total = text.Length();
		if (total == 0)
		{
			return false;
		}

		bool digits = false;
		for (int i = 0; i < total; i++)
		{
			string ch = text.Get(i);
			int code = ch.ToAscii();
			if (code >= 48 && code <= 57)
			{
				digits = true;
				continue;
			}

			bool sign = i == 0 && (ch == "-" || ch == "+");
			if (!sign && ch != ".")
			{
				return false;
			}
		}

		return digits;
	}

	// value with at most decimals decimals, no trailing zeros ("61.4", "61")
	static string RoundText(float value, int decimals)
	{
		float scale = Math.Pow(10, decimals);
		float rounded = Math.Round(value * scale) / scale;
		if (decimals == 0)
		{
			int whole = Math.Round(rounded);
			return whole.ToString();
		}

		return rounded.ToString();
	}

	protected string Unquote(string text)
	{
		int total = text.Length();
		if (total >= 2 && text.Substring(0, 1) == "\"" && text.Substring(total - 1, 1) == "\"")
		{
			return text.Substring(1, total - 2);
		}

		return text;
	}

	protected void SetPos(VPPWebhookNode tplNode, int pos)
	{
		int line = VPPWebhookText.LineOf(m_Source, pos);
		int col = VPPWebhookText.ColOf(m_Source, pos);
		tplNode.Line = line;
		tplNode.Col = col;
	}

	protected void AddIssue(bool isError, int pos, string code, string arg, string message)
	{
		int line = VPPWebhookText.LineOf(m_Source, pos);
		int col = VPPWebhookText.ColOf(m_Source, pos);
		AddIssueAt(isError, line, col, code, arg, message);
	}

	protected void AddIssueAt(bool isError, int line, int col, string code, string arg, string message)
	{
		VPPWebhookIssue issue = new VPPWebhookIssue();
		issue.IsError = isError;
		issue.Line = line;
		issue.Col = col;
		issue.Code = code;
		issue.Arg = arg;
		issue.Message = message;
		Issues.Insert(issue);
	}
};

class VPPWebhookText
{
	// 1-based line of a byte position.
	static int LineOf(string text, int pos)
	{
		int line = 1;
		int search = 0;
		int newline = 0;
		while (true)
		{
			newline = text.IndexOfFrom(search, "\n");
			if (newline < 0 || newline >= pos)
			{
				break;
			}

			line++;
			search = newline + 1;
		}

		return line;
	}

	// 1-based column (bytes) of a byte position.
	static int ColOf(string text, int pos)
	{
		int lineStart = 0;
		int search = 0;
		int newline = 0;
		while (true)
		{
			newline = text.IndexOfFrom(search, "\n");
			if (newline < 0 || newline >= pos)
			{
				break;
			}

			lineStart = newline + 1;
			search = newline + 1;
		}

		return pos - lineStart + 1;
	}
};
