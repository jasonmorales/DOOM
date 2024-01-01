export module input;

import std;
import nstd;

export namespace input {

using device_id = nstd::ename<
	"Keyboard", "Mouse"
>;

using event_id = nstd::ename<
	"None", "Any",

	"Backspace", "Tab", "Escape", "Space",
	"PageUp", "PageDown", "Home", "End", "Insert", "Delete",
	"UpArrow", "LeftArrow", "RightArrow", "DownArrow",
	"Enter", "AnyEnter",
	"Select", "Print", "Execute", "PrintScreen", "Pause", "Break", "Help", "Sleep", "Clear",

	"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"Plus", "Comma", "Minus", "Period", "Semicolon", "Slash", "Tilde", "LeftBracket", "BackSlash", "RightBracket", "Quote", "OEM8",

	"Numpad0", "Numpad1", "Numpad2", "Numpad3", "Numpad4", "Numpad5", "Numpad6", "Numpad7", "Numpad8", "Numpad9",
	"Multiply", "Add", "Separator", "Subtract", "Decimal", "Divide", "NumpadEnter",

	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",

	"CapsLock", "NumLock", "ScrollLock",

	"Shift", "LeftShift", "RightShift",
	"Ctrl", "LeftCtrl", "RightCtrl",
	"Alt", "LeftAlt", "RightAlt",
	"Win", "LeftWin", "RightWin",
	"Command", "LeftCommand", "RightCommand",

	"Apps", "LaunchApp1", "LaunchApp2", "LaunchMail", "LaunchMedia",
	"BrowserBack", "BrowserForward", "BrowserRefresh", "BrowserStop", "BrowserSearch", "BrowserFavorites", "BrowserHome",
	"VolumeMute", "VolumeUp", "VolumeDown",
	"MediaNext", "MediaPrev", "MediaStop", "MediaPlay",

	"MouseLeft", "MouseRight", "MouseCenter", "MouseX1", "MouseX2",
	"MouseLeftDbl", "MouseRightDbl", "MouseCenterDbl",
	"WheelDown", "WheelUp", "WheelVert", "WheelLeft", "WheelRight", "WheelHorz",

	"MouseAbsolute", "MouseDeltaX", "MouseDeltaY", "MouseDeltaXY",

	"Button1", "Button2", "Button3", "Button4", "Button5", "Button6", "Button7", "Button8",
	"Button9", "Button10", "Button11", "Button12", "Button13", "Button14", "Button15", "Button16",
	"Button17", "Button18", "Button19", "Button20", "Button21", "Button22", "Button23", "Button24",
	"Button25", "Button26", "Button27", "Button28", "Button29", "Button30", "Button31", "Button32",

	"ButtonA", "ButtonB", "ButtonX", "ButtonY",
	"ButtonTriangle", "ButtonCircle", "ButtonCross", "ButtonSquare",

	"ButtonL1", "ButtonL2", "ButtonL3",
	"ButtonR1", "ButtonR2", "ButtonR3",

	"ButtonBack", "ButtonStart",
	"ButtonMinus", "ButtonPlus",

	"JoyX", "JoyXPos", "JoyXNeg",
	"JoyY", "JoyYPos", "JoyYNeg",
	"JoyZ", "JoyZPos", "JoyZNeg",
	"JoyRX", "JoyRXPos", "JoyRXNeg",
	"JoyRY", "JoyRYPos", "JoyRYNeg",
	"JoyRZ", "JoyRZPos", "JoyRZNeg",

	"DPad4Up", "DPad4Left", "DPad4Right", "DPad4Down",
	"DPad8Up", "DPad8UpRight", "DPad8Right", "DPad8DownRight", "DPad8Down", "DPad8DownLeft", "DPad8Left", "DPad8UpLeft"
>;

using event_flags = nstd::flags<
	"up", "down",
	"changed", "increased", "decreased",
	"repeated", "held",
	"shift", "alt", "ctrl",
	"mouse_left", "mouse_right", "mouse_middle",
	"mouse_x1", "mouse_x2",
	"double_click",
	"shadow"
>;

struct event
{
	device_id device;
	int32 cursor_x = 0;
	int32 cursor_y = 0;
	event_id id;
	event_flags flags;
	wchar_t chr = 0;
	float value = 0;

	//constexpr event() noexcept = default;

	constexpr bool is(event_id test_id) const { return id == Any || id == test_id; }
	constexpr bool down(event_id test_id = Any) const { return flags.all<"down">() && is(test_id); }
	constexpr bool up(event_id test_id = Any) const { return flags.all<"up">() && is(test_id); }
	constexpr bool ctrl() const { return flags.all<"ctrl">(); }
	constexpr bool shift() const { return flags.all<"shift">(); }
	constexpr bool alt() const { return flags.all<"alt">(); }
	constexpr bool changed(event_id test_id = Any) const { return flags.all<"changed">() && is(test_id); }
	constexpr bool pressed(event_id test_id = Any) const { return down(test_id) && (changed() || repeated()); }
	constexpr bool released(event_id test_id = Any) const { return up(test_id) && changed(); }
	constexpr bool repeated(event_id test_id = Any) const { return flags.all<"repeated">() && is(test_id); }
	constexpr bool held(event_id test_id = Any) const { return flags.all<"held">() && is(test_id); }

	template<nstd::StringLiteralTemplate ID>
	constexpr bool is_device() const { return device.is<ID>(); }
	constexpr bool is_keyboard() const { return device.is<"Keyboard">(); }
	constexpr bool is_mouse() const { return device.is<"Mouse">(); }

	static constexpr auto Any = event_id{"Any"};
};

} // export namespace input