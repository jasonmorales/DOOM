export module platform.window;

import std;
import nstd;

export struct WindowDefinition
{
	bool isFullScreen = false;
	bool withBorder = false;
	bool withCaption = false;

	int32 x = 0;
	int32 y = 0;
	int32 width = 0;
	int32 height = 0;

	bool isResizeable = false;

	bool isExclusive = false;
	bool processDoubleClicks = false;

	string GetSignature() const
	{
		return std::format("doom__fs{}_bdr{}_cp{}_dclk{}",
			isFullScreen ? "X" : "_",
			withBorder ? "X" : "_",
			withCaption ? "X" : "_",
			processDoubleClicks ? "X" : "_");
	}
};
