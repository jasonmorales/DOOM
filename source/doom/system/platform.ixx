export module platform;

export import platform.window;
export import platform.debug;

#ifdef _WIN64
export import platform.win64;
export import platform.window.win64;
#endif
