/*
	XML Editor resolution scale.

	The window's Main panel is "scaled 1": exact sizes written in the layouts grow with the screen (one layout unit is
	one pixel at 1080p, about 1.33 pixels at 1440p). The tabs measure on screen and lay themselves out from script in
	those layout units. Above 1080p the engine did not read SetSize / SetPos values with the same scale as the layout,
	so script-placed widgets ended up smaller than the layout-defined ones around them and the tabs clipped into each
	other.

	Instead of assuming how the engine scales script values, Measure compares, at run time, a layout-defined exact
	widget (the 30-unit tab bar) with a probe sized from script (SetSize 100 x 100), and Size / Pos multiply the exact
	axes of every script size and position by the ratio. A fraction axis (hexactsize 0 / hexactpos 0) is passed as it
	is. At 1080p, or when the engine already agrees, the ratio is 1 and nothing changes.
*/
class VPPXEScale
{
	const static float PROBE_UNITS = 100;

	protected static float s_Ratio = 1.0;
	protected static bool s_Measured;

	static float Ratio()
	{
		return s_Ratio;
	}

	// layoutWidget: a layout-defined widget whose height is layoutUnits exact layout units and that script never
	// resizes. probe: an exact-size panel inside the same scaled panel. Returns true when the ratio changed.
	static bool Measure(Widget layoutWidget, float layoutUnits, Widget probe)
	{
		float refW = 0;
		float refH = 0;
		float probeW = 0;
		float probeH = 0;
		float layoutPixels = 0;
		float scriptPixels = 0;
		float ratio = 1.0;
		int screenW = 0;
		int screenH = 0;
		if (!layoutWidget || !probe || layoutUnits <= 0)
		{
			return false;
		}

		layoutWidget.GetScreenSize(refW, refH);
		if (refH <= 1)
		{
			return false;
		}

		probe.SetSize(PROBE_UNITS, PROBE_UNITS);
		probe.GetScreenSize(probeW, probeH);
		if (probeH <= 1)
		{
			return false;
		}

		layoutPixels = refH / layoutUnits;
		scriptPixels = probeH / PROBE_UNITS;
		ratio = layoutPixels / scriptPixels;
		if (Math.AbsFloat(ratio - 1.0) < 0.01)
		{
			ratio = 1.0;
		}

		if (s_Measured && Math.AbsFloat(ratio - s_Ratio) < 0.001)
		{
			return false;
		}

		s_Ratio = ratio;
		s_Measured = true;
		GetScreenSize(screenW, screenH);
		string screenText = screenW.ToString() + "x" + screenH.ToString();
		string layoutText = layoutPixels.ToString();
		string scriptText = scriptPixels.ToString();
		string ratioText = ratio.ToString();
		Print("[XMLEditor] UI scale at " + screenText + ": layout " + layoutText + " px per unit, script " + scriptText + " px per unit, ratio " + ratioText);
		return true;
	}

	// SetSize in layout units: exact axes are corrected by the ratio, fraction axes are passed as they are.
	static void Size(Widget target, float width, float height)
	{
		float setW = width;
		float setH = height;
		int flags = 0;
		if (!target)
		{
			return;
		}

		flags = target.GetFlags();
		if ((flags & WidgetFlags.HEXACTSIZE) != 0)
		{
			setW = width * s_Ratio;
		}

		if ((flags & WidgetFlags.VEXACTSIZE) != 0)
		{
			setH = height * s_Ratio;
		}

		target.SetSize(setW, setH);
	}

	// SetPos in layout units: exact axes are corrected by the ratio, fraction axes are passed as they are.
	static void Pos(Widget target, float x, float y)
	{
		float setX = x;
		float setY = y;
		int flags = 0;
		if (!target)
		{
			return;
		}

		flags = target.GetFlags();
		if ((flags & WidgetFlags.HEXACTPOS) != 0)
		{
			setX = x * s_Ratio;
		}

		if ((flags & WidgetFlags.VEXACTPOS) != 0)
		{
			setY = y * s_Ratio;
		}

		target.SetPos(setX, setY);
	}
};
