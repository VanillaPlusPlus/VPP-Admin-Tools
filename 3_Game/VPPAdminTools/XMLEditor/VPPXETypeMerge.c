// Engine merge semantics for types rows (INTERFACES v2.1 section 3). The same code runs on the server
// (index, save verification) and on the client (effective rows, staged edits), so both always agree.

class VPPXETypeMerge
{
	// Scalars UNSET, masks 0, Category -1, Present 0, FileIdx -1, DefCount 0.
	static VPPXETypeRow NewRow(string name)
	{
		VPPXETypeRow row = new VPPXETypeRow();
		row.Name = name;
		return row;
	}

	// The ADD template: nominal 0, lifetime TEMPLATE_LIFETIME, restock 0, min 0, quantmin -1, quantmax -1, cost TEMPLATE_COST, flags MAP.
	static VPPXETypeRow TemplateRow(string name)
	{
		VPPXETypeRow row = NewRow(name);
		row.Nominal = 0;
		row.Lifetime = VPPXEConst.TEMPLATE_LIFETIME;
		row.Restock = 0;
		row.Min = 0;
		row.QuantMin = -1;
		row.QuantMax = -1;
		row.Cost = VPPXEConst.TEMPLATE_COST;
		row.Flags = VPPXEFlagBit.MAP;
		row.Present = VPPXEField.NOMINAL | VPPXEField.MIN | VPPXEField.LIFETIME | VPPXEField.RESTOCK | VPPXEField.COST | VPPXEField.QUANTMIN | VPPXEField.QUANTMAX | VPPXEField.FLAGS;
		return row;
	}

	static VPPXETypeRow Copy(VPPXETypeRow src)
	{
		VPPXETypeRow row = new VPPXETypeRow();
		if (!src)
		{
			return row;
		}

		row.Name = src.Name;
		row.FileIdx = src.FileIdx;
		row.Line = src.Line;
		row.Present = src.Present;
		row.Nominal = src.Nominal;
		row.Min = src.Min;
		row.Lifetime = src.Lifetime;
		row.Restock = src.Restock;
		row.Cost = src.Cost;
		row.QuantMin = src.QuantMin;
		row.QuantMax = src.QuantMax;
		row.Flags = src.Flags;
		row.Category = src.Category;
		row.Usage = src.Usage;
		row.UsageUser = src.UsageUser;
		row.Value = src.Value;
		row.ValueUser = src.ValueUser;
		row.Tag = src.Tag;
		row.Issues = src.Issues;
		row.DefCount = src.DefCount;
		return row;
	}

	// Field-wise engine merge of one later definition over the accumulated row:
	// scalars, FLAGS and CATEGORY replace when present in over; USAGE/VALUE replace (both parts) only when the RESOLVED
	// mask is non-zero (a def naming only a user group that resolves to no known bit keeps the earlier list);
	// TAG replaces when non-zero. With limits null the raw masks (plain | user) decide.
	static void MergeInto(VPPXETypeRow acc, VPPXETypeRow over, VPPXELimits limits)
	{
		if (!acc || !over)
		{
			return;
		}

		for (int i = 0; i < 7; i++)
		{
			int scalarBit = FieldBitAt(i);
			if ((over.Present & scalarBit) != 0)
			{
				SetScalar(acc, scalarBit, GetScalar(over, scalarBit));
				acc.Present = acc.Present | scalarBit;
			}
		}

		if ((over.Present & VPPXEField.FLAGS) != 0)
		{
			acc.Flags = over.Flags;
			acc.Present = acc.Present | VPPXEField.FLAGS;
		}

		if ((over.Present & VPPXEField.CATEGORY) != 0)
		{
			acc.Category = over.Category;
			acc.Present = acc.Present | VPPXEField.CATEGORY;
		}

		int usageResolved = over.Usage | over.UsageUser;
		if (limits)
		{
			usageResolved = limits.EffectiveUsage(over.Usage, over.UsageUser);
		}

		if (usageResolved != 0)
		{
			acc.Usage = over.Usage;
			acc.UsageUser = over.UsageUser;
			acc.Present = acc.Present | VPPXEField.USAGE;
		}

		int valueResolved = over.Value | over.ValueUser;
		if (limits)
		{
			valueResolved = limits.EffectiveValue(over.Value, over.ValueUser);
		}

		if (valueResolved != 0)
		{
			acc.Value = over.Value;
			acc.ValueUser = over.ValueUser;
			acc.Present = acc.Present | VPPXEField.VALUE;
		}

		if (over.Tag != 0)
		{
			acc.Tag = over.Tag;
			acc.Present = acc.Present | VPPXEField.TAG;
		}

		acc.FileIdx = over.FileIdx;
		acc.DefCount = acc.DefCount + 1;
		acc.Issues = acc.Issues | over.Issues;
	}

	// UPDATE semantics only (any other Op leaves the row untouched). SetMask is applied first, then ClearMask:
	// SetMask scalars = edit value, FLAGS = FlagsValue, CATEGORY = FindCategory (-2 when unknown);
	// ClearMask scalar -> UNSET and bit off, CATEGORY -> -1 and bit off;
	// a list in REPLACE mode -> masks from the names (unknown names dropped), Present bit = mask != 0.
	static void ApplyEdit(VPPXETypeRow row, VPPXETypeEdit edit, VPPXELimits limits)
	{
		if (!row || !edit)
		{
			return;
		}

		if (edit.Op != VPPXEOp.UPDATE)
		{
			return;
		}

		for (int i = 0; i < 7; i++)
		{
			int scalarBit = FieldBitAt(i);
			if ((edit.SetMask & scalarBit) != 0)
			{
				SetScalar(row, scalarBit, EditScalar(edit, scalarBit));
				row.Present = row.Present | scalarBit;
			}
		}

		if ((edit.SetMask & VPPXEField.FLAGS) != 0)
		{
			row.Flags = edit.FlagsValue;
			row.Present = row.Present | VPPXEField.FLAGS;
		}

		if ((edit.SetMask & VPPXEField.CATEGORY) != 0)
		{
			int catIdx = -2;
			if (limits)
			{
				catIdx = limits.FindCategory(edit.Category);
			}

			if (catIdx < 0)
			{
				catIdx = -2;
			}

			row.Category = catIdx;
			row.Present = row.Present | VPPXEField.CATEGORY;
		}

		for (int j = 0; j < 7; j++)
		{
			int clearBit = FieldBitAt(j);
			if ((edit.ClearMask & clearBit) != 0)
			{
				SetScalar(row, clearBit, VPPXEConst.UNSET);
				row.Present = row.Present & ~clearBit;
			}
		}

		if ((edit.ClearMask & VPPXEField.CATEGORY) != 0)
		{
			row.Category = -1;
			row.Present = row.Present & ~VPPXEField.CATEGORY;
		}

		if (edit.UsageMode == VPPXEListMode.REPLACE)
		{
			row.Usage = 0;
			row.UsageUser = 0;
			if (limits)
			{
				row.Usage = limits.MaskOf(limits.Usages, edit.Usages);
				row.UsageUser = limits.MaskOf(limits.UsageGroups, edit.UsageUsers);
			}

			if ((row.Usage | row.UsageUser) != 0)
			{
				row.Present = row.Present | VPPXEField.USAGE;
			}
			else
			{
				row.Present = row.Present & ~VPPXEField.USAGE;
			}
		}

		if (edit.ValueMode == VPPXEListMode.REPLACE)
		{
			row.Value = 0;
			row.ValueUser = 0;
			if (limits)
			{
				row.Value = limits.MaskOf(limits.Values, edit.Values);
				row.ValueUser = limits.MaskOf(limits.ValueGroups, edit.ValueUsers);
			}

			if ((row.Value | row.ValueUser) != 0)
			{
				row.Present = row.Present | VPPXEField.VALUE;
			}
			else
			{
				row.Present = row.Present & ~VPPXEField.VALUE;
			}
		}

		if (edit.TagMode == VPPXEListMode.REPLACE)
		{
			row.Tag = 0;
			if (limits)
			{
				row.Tag = limits.MaskOf(limits.Tags, edit.Tags);
			}

			if (row.Tag != 0)
			{
				row.Present = row.Present | VPPXEField.TAG;
			}
			else
			{
				row.Present = row.Present & ~VPPXEField.TAG;
			}
		}
	}

	// Deterministic text of every field value plus Present: "n=..|mi=..|lt=..|rs=..|co=..|qn=..|qx=..|f=..|c=..|u=a/b|v=a/b|t=..|p=..".
	static string Signature(VPPXETypeRow r)
	{
		if (!r)
		{
			return "";
		}

		string sig = "n=" + r.Nominal.ToString();
		sig = sig + "|mi=" + r.Min.ToString();
		sig = sig + "|lt=" + r.Lifetime.ToString();
		sig = sig + "|rs=" + r.Restock.ToString();
		sig = sig + "|co=" + r.Cost.ToString();
		sig = sig + "|qn=" + r.QuantMin.ToString();
		sig = sig + "|qx=" + r.QuantMax.ToString();
		sig = sig + "|f=" + r.Flags.ToString();
		sig = sig + "|c=" + r.Category.ToString();
		sig = sig + "|u=" + r.Usage.ToString() + "/" + r.UsageUser.ToString();
		sig = sig + "|v=" + r.Value.ToString() + "/" + r.ValueUser.ToString();
		sig = sig + "|t=" + r.Tag.ToString();
		sig = sig + "|p=" + r.Present.ToString();
		return sig;
	}

	// VPPXEField bit for bit index 0..11, 0 outside.
	static int FieldBitAt(int index)
	{
		if (index < 0 || index >= VPPXEConst.FIELD_COUNT)
		{
			return 0;
		}

		return 1 << index;
	}

	// Bit index 0..11 of a single VPPXEField bit, -1 otherwise.
	static int FieldBitIndex(int fieldBit)
	{
		for (int i = 0; i < VPPXEConst.FIELD_COUNT; i++)
		{
			if (fieldBit == (1 << i))
			{
				return i;
			}
		}

		return -1;
	}

	static string FieldElementName(int fieldBit)
	{
		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				return "nominal";
			case VPPXEField.MIN:
				return "min";
			case VPPXEField.LIFETIME:
				return "lifetime";
			case VPPXEField.RESTOCK:
				return "restock";
			case VPPXEField.COST:
				return "cost";
			case VPPXEField.QUANTMIN:
				return "quantmin";
			case VPPXEField.QUANTMAX:
				return "quantmax";
			case VPPXEField.FLAGS:
				return "flags";
			case VPPXEField.CATEGORY:
				return "category";
			case VPPXEField.USAGE:
				return "usage";
			case VPPXEField.VALUE:
				return "value";
			case VPPXEField.TAG:
				return "tag";
		}

		return "";
	}

	// Value of one of the 7 scalar fields; UNSET for any other bit.
	static int GetScalar(VPPXETypeRow r, int fieldBit)
	{
		if (!r)
		{
			return VPPXEConst.UNSET;
		}

		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				return r.Nominal;
			case VPPXEField.MIN:
				return r.Min;
			case VPPXEField.LIFETIME:
				return r.Lifetime;
			case VPPXEField.RESTOCK:
				return r.Restock;
			case VPPXEField.COST:
				return r.Cost;
			case VPPXEField.QUANTMIN:
				return r.QuantMin;
			case VPPXEField.QUANTMAX:
				return r.QuantMax;
		}

		return VPPXEConst.UNSET;
	}

	// Sets one of the 7 scalar fields (does not touch Present); other bits are ignored.
	static void SetScalar(VPPXETypeRow r, int fieldBit, int value)
	{
		if (!r)
		{
			return;
		}

		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				r.Nominal = value;
				break;
			case VPPXEField.MIN:
				r.Min = value;
				break;
			case VPPXEField.LIFETIME:
				r.Lifetime = value;
				break;
			case VPPXEField.RESTOCK:
				r.Restock = value;
				break;
			case VPPXEField.COST:
				r.Cost = value;
				break;
			case VPPXEField.QUANTMIN:
				r.QuantMin = value;
				break;
			case VPPXEField.QUANTMAX:
				r.QuantMax = value;
				break;
		}
	}

	// Scalar value an edit carries for one of the 7 scalar bits; UNSET for any other bit.
	static int EditScalar(VPPXETypeEdit edit, int fieldBit)
	{
		if (!edit)
		{
			return VPPXEConst.UNSET;
		}

		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				return edit.Nominal;
			case VPPXEField.MIN:
				return edit.Min;
			case VPPXEField.LIFETIME:
				return edit.Lifetime;
			case VPPXEField.RESTOCK:
				return edit.Restock;
			case VPPXEField.COST:
				return edit.Cost;
			case VPPXEField.QUANTMIN:
				return edit.QuantMin;
			case VPPXEField.QUANTMAX:
				return edit.QuantMax;
		}

		return VPPXEConst.UNSET;
	}

	static bool IsScalarBit(int fieldBit)
	{
		int scalarMask = VPPXEField.NOMINAL | VPPXEField.MIN | VPPXEField.LIFETIME | VPPXEField.RESTOCK | VPPXEField.COST | VPPXEField.QUANTMIN | VPPXEField.QUANTMAX;
		if (fieldBit == 0)
		{
			return false;
		}

		if ((fieldBit & scalarMask) != fieldBit)
		{
			return false;
		}

		return FieldBitIndex(fieldBit) >= 0;
	}

	// VPPXEFlagBit for index 0..5, 0 outside.
	static int FlagBitAt(int index)
	{
		if (index < 0 || index >= 6)
		{
			return 0;
		}

		return 1 << index;
	}

	// XML attribute name of a flag bit: count_in_cargo, count_in_hoarder, count_in_map, count_in_player, crafted, deloot.
	static string FlagAttrName(int flagBit)
	{
		switch (flagBit)
		{
			case VPPXEFlagBit.CARGO:
				return "count_in_cargo";
			case VPPXEFlagBit.HOARDER:
				return "count_in_hoarder";
			case VPPXEFlagBit.MAP:
				return "count_in_map";
			case VPPXEFlagBit.PLAYER:
				return "count_in_player";
			case VPPXEFlagBit.CRAFTED:
				return "crafted";
			case VPPXEFlagBit.DELOOT:
				return "deloot";
		}

		return "";
	}

	// Comma-joined short names of the set flags in bit order ("cargo,map"); data text, not localized.
	static string FormatFlags(int flags)
	{
		string text = "";
		for (int i = 0; i < 6; i++)
		{
			int flagBit = 1 << i;
			if ((flags & flagBit) == 0)
			{
				continue;
			}

			string shortName = FlagShortName(flagBit);
			if (text == "")
			{
				text = shortName;
			}
			else
			{
				text = text + "," + shortName;
			}
		}

		return text;
	}

	static string FlagShortName(int flagBit)
	{
		switch (flagBit)
		{
			case VPPXEFlagBit.CARGO:
				return "cargo";
			case VPPXEFlagBit.HOARDER:
				return "hoarder";
			case VPPXEFlagBit.MAP:
				return "map";
			case VPPXEFlagBit.PLAYER:
				return "player";
			case VPPXEFlagBit.CRAFTED:
				return "crafted";
			case VPPXEFlagBit.DELOOT:
				return "deloot";
		}

		return "";
	}
};
