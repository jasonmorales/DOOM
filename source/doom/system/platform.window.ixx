export module platform.window;

import std;
import nstd;

export struct WindowClassDefinition
{
	bool processDoubleClicks = false;
	bool disableClose = false;

	int64 id = 0;

	wstring get_signature() const
	{
		return std::format(L"__nstd__wndlcass__dclk{}__nocls{}__",
			processDoubleClicks ? L"X" : L"_",
			disableClose ? L"X" : L"_");
	}
};

export struct WindowDefinition
{
	wstring title;

	int64 class_id = 0;

	int32 x = 0;
	int32 y = 0;
	int32 width = 0;
	int32 height = 0;

	bool acceptFileDrops = false;
	bool isResizeable = false;
	bool isExclusive = false;
	bool isFullScreen = false;
	bool withBorder = false;
	bool withCaption = false;
};
