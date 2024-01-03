#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOMINMAX
#define NOMENUS
#include <Windows.h>
#include <windowsx.h>
#undef CreateWindow
#undef RegisterClass
