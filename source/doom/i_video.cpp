//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include <winsock.h>

#include <GL/wglew.h>
#include <gl/gl.h>

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <algorithm>

#define POINTER_WARP_COUNTDOWN	1

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
boolean		grabMouse;
int		doPointerWarp = POINTER_WARP_COUNTDOWN;

template<typename T>
constexpr const GLenum gl_type = []{ static_assert(false, "type does not have a corresponding GL type"); return 0; }();
template<> constexpr const GLenum gl_type<int8> = GL_BYTE;
template<> constexpr const GLenum gl_type<uint8> = GL_UNSIGNED_BYTE;
template<> constexpr const GLenum gl_type<int16> = GL_SHORT;
template<> constexpr const GLenum gl_type<uint16> = GL_UNSIGNED_SHORT;
template<> constexpr const GLenum gl_type<int32> = GL_INT;
template<> constexpr const GLenum gl_type<uint32> = GL_UNSIGNED_INT;
template<> constexpr const GLenum gl_type<float> = GL_FLOAT;
template<> constexpr const GLenum gl_type<double> = GL_DOUBLE;

template<typename T>
class gl_vertex_attribute_builder
{
public:
    template<typename A>
    void add()
    {
        glEnableVertexAttribArray(index);
        if (std::is_integral_v<A>)
            glVertexAttribIPointer(index, size<A>(), gl_type<A>, sizeof(T), reinterpret_cast<GLvoid*>(offset));
        else
            glVertexAttribPointer(index, size<A>(), gl_type<A>, GL_FALSE, sizeof(T), reinterpret_cast<GLvoid*>(offset));
        offset += sizeof(A);
        ++index;
    };

private:
    size_t offset = 0;
    GLuint index = 0;

    template<typename A> requires is_arithmetic<A> GLint size() const { return 1; }
#if 0
    template<typename A> requires is_same<A, v2f> || is_same<A, v2d> GLint size() const { return 2; }
    template<typename A> requires is_same<A, v3f> || is_same<A, v3d> GLint size() const { return 3; }
    template<typename A> requires
        is_same<A, v4f> || is_same<A, v4> ||
        is_same<A, rgba_f> || is_same<A, rgba_d>
        GLint size() const { return 4; }
#endif
};

template<typename ...Ts>
class gl_vertex_attributes
{
public:
    static constexpr const GLint size = (sizeof(Ts)+...);

    static void add()
    {
        GLint index = 0;
        _add<Ts...>(index);
    }

    template<typename T, typename ...Ts>
    static void _add(GLint& index)
    {
        glEnableVertexAttribArray(index);
        index += 1;

        if constexpr (sizeof...(Ts) > 1)
            _add<Ts...>(index);
    }

private:
};

//  Translates the key currently in X_event
int xlatekey()
{

    int rc = 0;
#if 0
    switch (rc = XKeycodeToKeysym(X_display, X_event.xkey.keycode, 0))
    {
    case XK_Left:	rc = KEY_LEFTARROW;	break;
    case XK_Right:	rc = KEY_RIGHTARROW;	break;
    case XK_Down:	rc = KEY_DOWNARROW;	break;
    case XK_Up:	rc = KEY_UPARROW;	break;
    case XK_Escape:	rc = KEY_ESCAPE;	break;
    case XK_Return:	rc = KEY_ENTER;		break;
    case XK_Tab:	rc = KEY_TAB;		break;
    case XK_F1:	rc = KEY_F1;		break;
    case XK_F2:	rc = KEY_F2;		break;
    case XK_F3:	rc = KEY_F3;		break;
    case XK_F4:	rc = KEY_F4;		break;
    case XK_F5:	rc = KEY_F5;		break;
    case XK_F6:	rc = KEY_F6;		break;
    case XK_F7:	rc = KEY_F7;		break;
    case XK_F8:	rc = KEY_F8;		break;
    case XK_F9:	rc = KEY_F9;		break;
    case XK_F10:	rc = KEY_F10;		break;
    case XK_F11:	rc = KEY_F11;		break;
    case XK_F12:	rc = KEY_F12;		break;

    case XK_BackSpace:
    case XK_Delete:	rc = KEY_BACKSPACE;	break;

    case XK_Pause:	rc = KEY_PAUSE;		break;

    case XK_KP_Equal:
    case XK_equal:	rc = KEY_EQUALS;	break;

    case XK_KP_Subtract:
    case XK_minus:	rc = KEY_MINUS;		break;

    case XK_Shift_L:
    case XK_Shift_R:
        rc = KEY_RSHIFT;
        break;

    case XK_Control_L:
    case XK_Control_R:
        rc = KEY_RCTRL;
        break;

    case XK_Alt_L:
    case XK_Meta_L:
    case XK_Alt_R:
    case XK_Meta_R:
        rc = KEY_RALT;
        break;

    default:
        if (rc >= XK_space && rc <= XK_asciitilde)
            rc = rc - XK_space + ' ';
        if (rc >= 'A' && rc <= 'Z')
            rc = rc - 'A' + 'a';
        break;
    }
#endif

    return rc;
}

void I_ShutdownGraphics()
{
#if 0
    // Detach from X server
    if (!XShmDetach(X_display, &X_shminfo))
        I_Error("XShmDetach() failed in I_ShutdownGraphics()");

    // Release shared memory.
    shmdt(X_shminfo.shmaddr);
    shmctl(X_shminfo.shmid, IPC_RMID, 0);

    // Paranoia.
    image->data = nullptr;
#endif
}

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		mousemoved = false;
boolean		shmFinished;

void I_GetEvent()
{
#if 0
    event_t event;

    // put event-grabbing stuff in here
    XNextEvent(X_display, &X_event);
    switch (X_event.type)
    {
    case KeyPress:
        event.type = ev_keydown;
        event.data1 = xlatekey();
        D_PostEvent(&event);
        // fprintf(stderr, "k");
        break;
    case KeyRelease:
        event.type = ev_keyup;
        event.data1 = xlatekey();
        D_PostEvent(&event);
        // fprintf(stderr, "ku");
        break;
    case ButtonPress:
        event.type = ev_mouse;
        event.data1 =
            (X_event.xbutton.state & Button1Mask)
            | (X_event.xbutton.state & Button2Mask ? 2 : 0)
            | (X_event.xbutton.state & Button3Mask ? 4 : 0)
            | (X_event.xbutton.button == Button1)
            | (X_event.xbutton.button == Button2 ? 2 : 0)
            | (X_event.xbutton.button == Button3 ? 4 : 0);
        event.data2 = event.data3 = 0;
        D_PostEvent(&event);
        // fprintf(stderr, "b");
        break;
    case ButtonRelease:
        event.type = ev_mouse;
        event.data1 =
            (X_event.xbutton.state & Button1Mask)
            | (X_event.xbutton.state & Button2Mask ? 2 : 0)
            | (X_event.xbutton.state & Button3Mask ? 4 : 0);
        // suggest parentheses around arithmetic in operand of |
        event.data1 =
            event.data1
            ^ (X_event.xbutton.button == Button1 ? 1 : 0)
            ^ (X_event.xbutton.button == Button2 ? 2 : 0)
            ^ (X_event.xbutton.button == Button3 ? 4 : 0);
        event.data2 = event.data3 = 0;
        D_PostEvent(&event);
        // fprintf(stderr, "bu");
        break;
    case MotionNotify:
        event.type = ev_mouse;
        event.data1 =
            (X_event.xmotion.state & Button1Mask)
            | (X_event.xmotion.state & Button2Mask ? 2 : 0)
            | (X_event.xmotion.state & Button3Mask ? 4 : 0);
        event.data2 = (X_event.xmotion.x - lastmousex) << 2;
        event.data3 = (lastmousey - X_event.xmotion.y) << 2;

        if (event.data2 || event.data3)
        {
            lastmousex = X_event.xmotion.x;
            lastmousey = X_event.xmotion.y;
            if (X_event.xmotion.x != X_width / 2 &&
                X_event.xmotion.y != X_height / 2)
            {
                D_PostEvent(&event);
                // fprintf(stderr, "m");
                mousemoved = false;
            }
            else
            {
                mousemoved = true;
            }
        }
        break;

    case Expose:
    case ConfigureNotify:
        break;

    default:
        if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
        break;
    }
#endif
}

#if 0
Cursor
createnullcursor
(Display* display,
    Window	root)
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc = XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
        &dummycolour, &dummycolour, 0, 0);
    XFreePixmap(display, cursormask);
    XFreeGC(display, gc);
    return cursor;
}
#endif

void I_UpdateNoBlit()
{
}

void Video::FinishUpdate()
{
    static time_t lasttic = 0;

    // UNUSED static unsigned char *bigscreen=0;

    // draws little dots on the bottom of the screen
    if (devparm)
    {

        auto i = I_GetTime();
        auto tics = i - lasttic;
        lasttic = i;
        if (tics > 20) tics = 20;

        for (i = 0; i < tics * 2; i += 2)
            screens[0][(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0xff;
        for (; i < 20 * 2; i += 2)
            screens[0][(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0x0;

    }

    SwapBuffers(deviceContext);

#if 0
    // scales the screen size before blitting it
    if (screenMultiply == 2)
    {
        unsigned int* olineptrs[2];
        unsigned int* ilineptr;
        int x, y, i;
        unsigned int twoopixels;
        unsigned int twomoreopixels;
        unsigned int fouripixels;

        ilineptr = (unsigned int*)(screens[0]);
        for (i = 0; i < 2; i++)
            olineptrs[i] = (unsigned int*)&image->data[i * X_width];

        y = SCREENHEIGHT;
        while (y--)
        {
            x = SCREENWIDTH;
            do
            {
                fouripixels = *ilineptr++;
                twoopixels = (fouripixels & 0xff000000)
                    | ((fouripixels >> 8) & 0xffff00)
                    | ((fouripixels >> 16) & 0xff);
                twomoreopixels = ((fouripixels << 16) & 0xff000000)
                    | ((fouripixels << 8) & 0xffff00)
                    | (fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
                * olineptrs[0]++ = twoopixels;
                *olineptrs[1]++ = twoopixels;
                *olineptrs[0]++ = twomoreopixels;
                *olineptrs[1]++ = twomoreopixels;
#else
                * olineptrs[0]++ = twomoreopixels;
                *olineptrs[1]++ = twomoreopixels;
                *olineptrs[0]++ = twoopixels;
                *olineptrs[1]++ = twoopixels;
#endif
            } while (x -= 4);
            olineptrs[0] += X_width / 4;
            olineptrs[1] += X_width / 4;
        }

    }
    else if (screenMultiply == 3)
    {
        unsigned int* olineptrs[3];
        unsigned int* ilineptr;
        int x, y, i;
        unsigned int fouropixels[3];
        unsigned int fouripixels;

        ilineptr = (unsigned int*)(screens[0]);
        for (i = 0; i < 3; i++)
            olineptrs[i] = (unsigned int*)&image->data[i * X_width];

        y = SCREENHEIGHT;
        while (y--)
        {
            x = SCREENWIDTH;
            do
            {
                fouripixels = *ilineptr++;
                fouropixels[0] = (fouripixels & 0xff000000)
                    | ((fouripixels >> 8) & 0xff0000)
                    | ((fouripixels >> 16) & 0xffff);
                fouropixels[1] = ((fouripixels << 8) & 0xff000000)
                    | (fouripixels & 0xffff00)
                    | ((fouripixels >> 8) & 0xff);
                fouropixels[2] = ((fouripixels << 16) & 0xffff0000)
                    | ((fouripixels << 8) & 0xff00)
                    | (fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
                * olineptrs[0]++ = fouropixels[0];
                *olineptrs[1]++ = fouropixels[0];
                *olineptrs[2]++ = fouropixels[0];
                *olineptrs[0]++ = fouropixels[1];
                *olineptrs[1]++ = fouropixels[1];
                *olineptrs[2]++ = fouropixels[1];
                *olineptrs[0]++ = fouropixels[2];
                *olineptrs[1]++ = fouropixels[2];
                *olineptrs[2]++ = fouropixels[2];
#else
                * olineptrs[0]++ = fouropixels[2];
                *olineptrs[1]++ = fouropixels[2];
                *olineptrs[2]++ = fouropixels[2];
                *olineptrs[0]++ = fouropixels[1];
                *olineptrs[1]++ = fouropixels[1];
                *olineptrs[2]++ = fouropixels[1];
                *olineptrs[0]++ = fouropixels[0];
                *olineptrs[1]++ = fouropixels[0];
                *olineptrs[2]++ = fouropixels[0];
#endif
            } while (x -= 4);
            olineptrs[0] += 2 * X_width / 4;
            olineptrs[1] += 2 * X_width / 4;
            olineptrs[2] += 2 * X_width / 4;
        }

    }
    else if (screenMultiply == 4)
    {
        // Broken. Gotta fix this some day.
        void Expand4(unsigned*, double*);
        Expand4((unsigned*)(screens[0]), (double*)(image->data));
    }

    if (doShm)
    {

        if (!XShmPutImage(X_display,
            X_mainWindow,
            X_gc,
            image,
            0, 0,
            0, 0,
            X_width, X_height,
            True))
            I_Error("XShmPutImage() failed\n");

        // wait for it to finish and processes all input events
        shmFinished = false;
        do
        {
            I_GetEvent();
        } while (!shmFinished);

    }
    else
    {

        // draw the image
        XPutImage(X_display,
            X_mainWindow,
            X_gc,
            image,
            0, 0,
            0, 0,
            X_width, X_height);

        // sync up with server
        XSync(X_display, False);

    }
#endif
}

void I_ReadScreen(byte* scr)
{
    memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

// Palette stuff.
#if 0
static XColor	colors[256];

void UploadNewPalette(Colormap cmap, byte* palette)
{

    register int	i;
    register int	c;
    static boolean	firstcall = true;

#ifdef __cplusplus
    if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
    if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
    {
        // initialize the colormap
        if (firstcall)
        {
            firstcall = false;
            for (i = 0; i < 256; i++)
            {
                colors[i].pixel = i;
                colors[i].flags = DoRed | DoGreen | DoBlue;
            }
        }

        // set the X colormap entries
        for (i = 0; i < 256; i++)
        {
            c = gammatable[usegamma][*palette++];
            colors[i].red = (c << 8) + c;
            c = gammatable[usegamma][*palette++];
            colors[i].green = (c << 8) + c;
            c = gammatable[usegamma][*palette++];
            colors[i].blue = (c << 8) + c;
        }

        // store the colors to the current colormap
        XStoreColors(X_display, cmap, colors, 256);

    }
}
#endif

//
// I_SetPalette
//
void I_SetPalette([[maybe_unused]] byte* palette)
{
    //UploadNewPalette(X_cmap, palette);
}

#if 0
//
// This function is probably redundant,
//  if XShmDetach works properly.
// ddt never detached the XShm memory,
//  thus there might have been stale
//  handles accumulating.
//
void grabsharedmemory(int size)
{

    int			key = ('d' << 24) | ('o' << 16) | ('o' << 8) | 'm';
    struct shmid_ds	shminfo;
    int			minsize = 320 * 200;
    int			id;
    int			rc;
    // UNUSED int done=0;
    int			pollution = 5;

    // try to use what was here before
    do
    {
        id = shmget((key_t)key, minsize, 0777); // just get the id
        if (id != -1)
        {
            rc = shmctl(id, IPC_STAT, &shminfo); // get stats on it
            if (!rc)
            {
                if (shminfo.shm_nattch)
                {
                    fprintf(stderr, "User %d appears to be running "
                        "DOOM.  Is that wise?\n", shminfo.shm_cpid);
                    key++;
                }
                else
                {
                    if (getuid() == shminfo.shm_perm.cuid)
                    {
                        rc = shmctl(id, IPC_RMID, 0);
                        if (!rc)
                            fprintf(stderr,
                                "Was able to kill my old shared memory\n");
                        else
                            I_Error("Was NOT able to kill my old shared memory");

                        id = shmget((key_t)key, size, IPC_CREAT | 0777);
                        if (id == -1)
                            I_Error("Could not get shared memory");

                        rc = shmctl(id, IPC_STAT, &shminfo);

                        break;

                    }
                    if (size >= shminfo.shm_segsz)
                    {
                        fprintf(stderr,
                            "will use %d's stale shared memory\n",
                            shminfo.shm_cpid);
                        break;
                    }
                    else
                    {
                        fprintf(stderr,
                            "warning: can't use stale "
                            "shared memory belonging to id %d, "
                            "key=0x%x\n",
                            shminfo.shm_cpid, key);
                        key++;
                    }
                }
            }
            else
            {
                I_Error("could not get stats on key=%d", key);
            }
        }
        else
        {
            id = shmget((key_t)key, size, IPC_CREAT | 0777);
            if (id == -1)
            {
                extern int errno;
                fprintf(stderr, "errno=%d\n", errno);
                I_Error("Could not get any shared memory");
            }
            break;
        }
    } while (--pollution);

    if (!pollution)
    {
        I_Error("Sorry, system too polluted with stale "
            "shared memory segments.\n");
    }

    X_shminfo.shmid = id;

    // attach to the shared memory segment
    image->data = X_shminfo.shmaddr = shmat(id, 0, 0);

    fprintf(stderr, "shared memory id=%d, addr=0x%x\n", id,
        (int)(image->data));
}
#endif

// This is the default window proc that all windows use after being initialized. It
// simply looks up the associated Window class and then passes the event parameters
// along to the custom function.
LRESULT CALLBACK GlobalWindowProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* window = reinterpret_cast<Video*>(GetWindowLongPtr(handle, GWLP_USERDATA));
    return window->HandleSystemEvent({handle, msg, wParam, lParam});
}

// This is the default window proc that all windows use during initialization. It is
// meant to handle the WM_NCCREATE message, which comes prior to even WM_CREATE. This
// message carries a pointer to the Window class that this window belongs to, so it
// can be associated. It then changes over to the simpler window proc, above.
LRESULT CALLBACK GlobalInitWindowProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCT create(reinterpret_cast<LPCREATESTRUCT>(lParam));

        // Assign the pointer to the Window class that owns this window to the "user data" of this window
        SetLastError(0);
        auto result(SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams)));
        auto error_core = GetLastError();
        if (result == 0 && error_core != 0)
        {
            LPWSTR buffer(NULL);
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
            MessageBoxW(nullptr, buffer, L"Error in WM_NCCREATE", MB_OK);
            LocalFree(buffer);
            return FALSE;
        }

        // Replace this window proc with the simpler version which passes the event to the Window class
        SetLastError(0);
        result = SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GlobalWindowProc));
        error_core = GetLastError();
        if (result == 0 && error_core != 0)
        {
            LPWSTR buffer(NULL);
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
            MessageBoxW(nullptr, buffer, L"Error in WM_NCCREATE", MB_OK);
            LocalFree(buffer);
            return FALSE;
        }
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

void GLAPIENTRY Video::GLErrorCallback(
    [[maybe_unused]] GLenum source,
    [[maybe_unused]] GLenum type,
    [[maybe_unused]] GLuint id,
    [[maybe_unused]] GLenum severity,
    [[maybe_unused]] GLsizei length,
    [[maybe_unused]] const GLchar* message,
    [[maybe_unused]] const void* param)
{
    /*std::stringstream out;
    switch (type)
    {
    case GL_DEBUG_SOURCE_API:
        out << "[API] ";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        out << "[WINDOW SYSTEM] ";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        out << "[SHADER COMPILER] ";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        out << "[THIRD PARTY] ";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        out << "[APPLICATION] ";
        break;
    case GL_DEBUG_SOURCE_OTHER:
    default:
        out << "[???] ";
        break;
    }
    out << message;
    Phantasus::Debug::Output(string{out.str().c_str()});
    Phantasus::Debug::Output("\n");
    if ((type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
        && (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW))
        DEBUG_BREAK();*/
}

void Video::Init()
{
    if (isInitialized)
        return;

    isInitialized = true;

    CommandLine::TryGetValues("-multiply", screenMultiply);

    windowWidth *= screenMultiply;
    windowHeight *= screenMultiply;

    isFullScreen = CommandLine::HasArg("-fullscreen");
    isExclusive = CommandLine::HasArg("-exclusive");
    isBorderless = CommandLine::HasArg("-borderless");
    isResizeable = CommandLine::HasArg("-resizeable");

    if (isFullScreen && isExclusive)
    {
        DEVMODE mode;
        ZeroMemory(&mode, sizeof(DEVMODE));
        mode.dmSize = sizeof(DEVMODE);
        mode.dmPelsWidth = windowWidth;
        mode.dmPelsHeight = windowHeight;
        mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

        DWORD display_flags = CDS_FULLSCREEN;
        ChangeDisplaySettings(&mode, display_flags);
    }

    if (!RegisterWindowClass())
        I_Error("Failed to register window class");


    // Figure out the dimensions of the window, factoring in style elements like
    // border and caption bar
    windowStyle = isBorderless ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    windowStyle |= WS_CAPTION;

    if (isFullScreen)
        windowStyle |= WS_MAXIMIZE;

    if (isResizeable)
        windowStyle |= WS_THICKFRAME;
    else
        windowStyle &= ~WS_THICKFRAME;

    auto dpi = GetDpiForSystem();
    screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, dpi);
    screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, dpi);
    //SystemParametersInfoForDpi(SPI_GETWORKAREA, 
    windowWidth = std::min(screenWidth, windowWidth);
    windowHeight = std::min(screenHeight, windowHeight);

    if (isFullScreen)
    {
        RECT rec_non_client = {};
        AdjustWindowRectExForDpi(&rec_non_client, windowStyle, FALSE, 0, dpi);
        windowWidth = screenWidth - (rec_non_client.right - rec_non_client.left);
        windowHeight = screenHeight - (rec_non_client.bottom - rec_non_client.top);
    }

    RECT rec;
    rec.left = 0;
    rec.right = windowWidth;
    rec.top = 0;
    rec.bottom = windowHeight;
    AdjustWindowRectExForDpi(&rec, windowStyle, FALSE, 0, dpi);

    // Create the window
    windowHandle = ::CreateWindowExW(
        windowStyleEx,
        WindowClassName,
        L"Doom",
        windowStyle,
        isFullScreen ? 0 : CW_USEDEFAULT,
        isFullScreen ? 0 : CW_USEDEFAULT,
        rec.right - rec.left,
        rec.bottom - rec.top,
        nullptr,
        nullptr,
        0,
        this);

    RECT actual = {};
    GetClientRect(windowHandle, &actual);
    windowWidth = actual.right - actual.left;
    windowHeight = actual.bottom - actual.top;

    if (windowHandle == nullptr)
    {
        auto error_core = GetLastError();
        LPWSTR buffer = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
        MessageBoxW(nullptr, buffer, L"Error in CreateWindowW", MB_OK);
        LocalFree(buffer);
        return;
    }

    // Get a device context for the window
    deviceContext = GetDC(windowHandle);

    // Set the pixel format
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = 0
        | PFD_DRAW_TO_WINDOW
        | PFD_SUPPORT_OPENGL
        | PFD_DOUBLEBUFFER
        | PFD_DEPTH_DONTCARE
        | PFD_TYPE_RGBA
        ;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 0;
    pfd.cAccumBits = 0;
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 8;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    auto pixel_format = ChoosePixelFormat(deviceContext, &pfd);
    SetPixelFormat(deviceContext, pixel_format, &pfd);

    auto dummyContext = wglCreateContext(deviceContext);
    wglMakeCurrent(deviceContext, dummyContext);

    glewInit();

    // Create the render context
    int attributes[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        //WGL_CONTEXT_LAYER_PLANE_ARB, 0,
        //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
        //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB | WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0
    };

    renderContext = wglCreateContextAttribsARB(deviceContext, nullptr, attributes);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyContext);

    wglMakeCurrent(deviceContext, renderContext);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLErrorCallback, 0);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    glFrontFace(GL_CCW);

    glDisable(GL_MULTISAMPLE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch (status)
    {
    case GL_FRAMEBUFFER_UNDEFINED:
        I_Error("Video::Init - framebuffer undefined");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        I_Error("Video::Init - framebuffer incomplete attachment\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        I_Error("Video::Init - framebuffer incomplete missing attachment\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        I_Error("Video::Init - framebuffer incomplete draw buffer\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        I_Error("Video::Init - framebuffer incomplete read buffer\n");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        I_Error("Video::Init - framebuffer unsupported\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        I_Error("Video::Init - framebuffer incomplete multisample\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        I_Error("Video::Init - framebuffer incomplete layer targets\n");
        break;
    default:
        I_Error(string("Video::Init - error: ") + std::to_string(status).c_str() + "\n");
        break;
    case GL_FRAMEBUFFER_COMPLETE:
        break;
    }

    float screenVertices[] =
    {
        -1.f,  1.f,  0.f, 1.f,
        -1.f, -1.f,  0.f, 0.f,
        1.f, -1.f,  1.f, 0.f,

        -1.f,  1.f,  0.f, 1.f,
        1.f, -1.f,  1.f, 0.f,
        1.f,  1.f,  1.f, 1.f,
    };

    glGenVertexArrays(1, &screenVAO);
    glBindVertexArray(screenVAO);

    glGenBuffers(1, &screenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

    gl_vertex_attribute_builder<float[4]> screen_vertex_attribs;
    //screen_vertex_attribs.add<v2f>();
    //screen_vertex_attribs.add<v2f>();

    constexpr auto A = gl_vertex_attributes<float, float, double, byte>::size;
    gl_vertex_attributes<float, float, double, byte>::add();

    //constexpr auto Y = gl_vertex_attributes<float>::calc();

    ShowWindow(windowHandle, SW_SHOWNORMAL);
}

void Video::StartFrame()
{
    glClearColor(1.f, 0.f, 0.f, 0.f);
    //glClearStencil(0);
    //glStencilMask(0xff);
    glClear(GL_COLOR_BUFFER_BIT /*| GL_STENCIL_BUFFER_BIT*/);
}

void Video::StartTick()
{
#if 0
    if (!X_display)
        return;

    while (XPending(X_display))
        I_GetEvent();

    // Warp the pointer back to the middle of the window
    //  or it will wander off - that is, the game will
    //  loose input focus within X11.
    if (grabMouse)
    {
        if (!--doPointerWarp)
        {
            XWarpPointer(X_display,
                None,
                X_mainWindow,
                0, 0,
                0, 0,
                X_width / 2, X_height / 2);

            doPointerWarp = POINTER_WARP_COUNTDOWN;
        }
    }

    mousemoved = false;
#endif
}

LRESULT Video::HandleSystemEvent(const SystemEvent& event)
{
#if 0
    using Input::Button;

    auto get_modifier_key_flags = [w_param = event.wParam]() -> Input::Event::Flag
        {
            Input::Event::Flag out;
            if ((w_param & MK_CONTROL) != 0) out.set(Input::Event::Flag::Ctrl);
            if ((w_param & MK_SHIFT) != 0) out.set(Input::Event::Flag::Shift);
            if ((GetKeyState(VK_MENU) & 0x8000) != 0) out.set(Input::Event::Flag::Alt);
            if ((w_param & MK_LBUTTON) != 0) out.set(Input::Event::Flag::MouseLeft);
            if ((w_param & MK_MBUTTON) != 0) out.set(Input::Event::Flag::MouseMiddle);
            if ((w_param & MK_RBUTTON) != 0) out.set(Input::Event::Flag::MouseRight);
            if ((w_param & MK_XBUTTON1) != 0) out.set(Input::Event::Flag::MouseX1);
            if ((w_param & MK_XBUTTON2) != 0) out.set(Input::Event::Flag::MouseX2);
            return out;
        };

    auto cursor_pos = [this, l_param = event.lParam](bool absolute = false) -> v2i
        {
            if (!absolute)
                return {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};

            auto client = GetClientArea();
            return {GET_X_LPARAM(l_param) - client.left(), GET_Y_LPARAM(l_param) - client.top()};
        };

    auto handle_mouse_wheel = [&, w_param = event.wParam](Input::Button key)
        {
            auto count = GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA;
            key = enum_offset(key, sign(count));
            count = std::abs(count);

            for (int i(0); i < count; ++i)
            {
                input->AddEvent(Input::Device::Mouse, key, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos(true));
            }
        };
#endif
    switch (event.message)
    {
    case WM_ACTIVATE:
        hasFocus = (LOWORD(event.wParam) != WA_INACTIVE);
        //input->OnFocusChanged(has_focus);
        break;

    case WM_SETFOCUS:
        //input->OnFocusChanged(true);
        break;
    case WM_KILLFOCUS:
        //input->OnFocusChanged(false);
        break;

    case WM_MOUSEACTIVATE:
        return MA_ACTIVATEANDEAT;

    case WM_CREATE:
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(event.handle, &ps);
        EndPaint(event.handle, &ps);
    }
    return 0;

    case WM_SIZING:
    {
        RECT actual = {};
        GetClientRect(windowHandle, &actual);
        windowWidth = actual.right - actual.left;
        windowHeight = actual.bottom - actual.top;
        //display->SetDimensions(width, height);
        return TRUE;
    }
    case WM_SIZE:
    {
        RECT actual = {};
        GetClientRect(windowHandle, &actual);
        windowWidth = actual.right - actual.left;
        windowHeight = actual.bottom - actual.top;
        //display->SetDimensions(width, height);
        return 0;
    }
    case WM_DROPFILES:
        break;

    case WM_DEVICECHANGE:
        /*LOG(Core, Debug, "WM_DEVICECHANGE" + "\n"
        + "hwnd = " + std::to_string(reinterpret_cast<intptr_t>(event.handle)).c_str() + "\n"
        + "msg = " + std::to_string(event.message).c_str() + "\n"
        + "wparam = " + std::to_string(event.wParam).c_str() + "\n"
        + "lparam = " + std::to_string(event.lParam).c_str() + "\n");
        input->ScanForDevices();*/
        break;

    case WM_SYSCOMMAND:
        switch (event.wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
        }	
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

#if 0
    case WM_MOUSEWHEEL:
        handle_mouse_wheel(Button::MouseWheel_V);
        return 0;

    case WM_MOUSEHWHEEL:
        handle_mouse_wheel(Button::MouseWheel_H);
        return 0;

    case WM_LBUTTONDOWN:
        input->AddEvent(Input::Device::Mouse, Button::MouseLeft, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_LBUTTONUP:
        input->AddEvent(Input::Device::Mouse, Button::MouseLeft, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_RBUTTONDOWN:
        input->AddEvent(Input::Device::Mouse, Button::MouseRight, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_RBUTTONUP:
        input->AddEvent(Input::Device::Mouse, Button::MouseRight, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_MBUTTONDOWN:
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddle, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_MBUTTONUP:
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddle, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_XBUTTONDOWN:
        if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON1)
            input->AddEvent(Input::Device::Mouse, Button::MouseX1, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        else if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON2)
            input->AddEvent(Input::Device::Mouse, Button::MouseX2, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return TRUE;
    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON1)
            input->AddEvent(Input::Device::Mouse, Button::MouseX1, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        else if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON2)
            input->AddEvent(Input::Device::Mouse, Button::MouseX2, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return TRUE;

    case WM_LBUTTONDBLCLK:
        input->AddEvent(Input::Device::Mouse, Button::MouseLeftDbl, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        input->AddEvent(Input::Device::Mouse, Button::MouseLeft, Input::Event::Flag::DoubleClick | Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_RBUTTONDBLCLK:
        input->AddEvent(Input::Device::Mouse, Button::MouseRightDbl, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        input->AddEvent(Input::Device::Mouse, Button::MouseRight, Input::Event::Flag::DoubleClick | Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_MBUTTONDBLCLK:
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddleDbl, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddle, Input::Event::Flag::DoubleClick | Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (event.wParam == VK_SHIFT)
            return 0;

        uint32 scan_code = (event.lParam >> 16) & 0xff;
        auto extended = TestBit(event.lParam, 24);
        auto key = input->TranslateButton(event.wParam, scan_code, extended);

        /*
        std::stringstream msg;
        msg << "WM_KEYDOWN " << event.message << " " << event.wParam << " " << event.lParam << " " << scan_code << " " << extended << " " << enum_string(key) << " " << TestBit(event.lParam, 30) << "\n";
        Host::DebugOutput(msg.str());
        /**/

        auto count = event.lParam & 0xffff;
        Input::Event::Flag flags = Input::Event::Flag::Down;
        auto alt_down = (HIWORD(event.lParam) & KF_ALTDOWN);
        if (alt_down)
            flags.set(Input::Event::Flag::Alt);
        if (TestBit(event.lParam, 30))
            flags.set(Input::Event::Flag::Repeated);
        else
            flags.set(Input::Event::Flag::Changed);
        for (int i(0); i < count; ++i)
        {
            input->AddEvent(Input::Device::Keyboard, key, flags);
        }
    }
    return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        if (event.wParam == VK_SHIFT)
            return 0;

        uint32 scan_code = (event.lParam >> 16) & 0xff;
        auto extended = TestBit(event.lParam, 24);
        auto key = input->TranslateButton(event.wParam, scan_code, extended);

        /*
        std::stringstream msg;
        msg << "WM_KEYUP " << event.message << " " << event.wParam << " " << event.lParam << " " << scan_code << " " << extended << " " << enum_string(key) << "\n";
        Host::DebugOutput(msg.str());
        /**/

        input->AddEvent(Input::Device::Keyboard, key, Input::Event::Flag::Up | Input::Event::Flag::Changed);
    }
    return 0;

    case WM_UNICHAR:
    case WM_CHAR:
    case WM_SYSCHAR:
    {
        int count(event.lParam & 0xffff);
        for (int i(0); i < count; ++i)
        {
            input->AddCharacterToStream(Input::Device::Keyboard, wchar_t(event.wParam));
        }
    }
    return 0;
#endif
    }

    return DefWindowProc(event.handle, event.message, event.wParam, event.lParam);
}

void Video::DeliverSystemMessages()
{
    // TODO: This could be in a thread
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
    {
        // Check for a WM_QUIT message
        auto result(GetMessage(&msg, nullptr, 0, 0));
        if (result == -1)
            continue;

        if (result == 0)
            I_Quit();

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool Video::RegisterWindowClass()
{
    // TODO: Build this from non OS specific data passed into the function, then check
    //   to see if a class with that "signature" already exists or not, skipping
    //   registration if it is unnecessary
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = GlobalInitWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(Video*);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = WindowClassName;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (::RegisterClassEx(&wc) == 0)
    {
        auto error_core = GetLastError();
        if (error_core != ERROR_CLASS_ALREADY_EXISTS)
        {
            LPWSTR buffer(NULL);
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
            MessageBoxW(nullptr, buffer, L"Error in RegisterClassEx", MB_OK);
            LocalFree(buffer);
            return false;
        }
    }

    return true;
}

unsigned	exptable[256];

void InitExpand()
{
    int		i;

    for (i = 0; i < 256; i++)
        exptable[i] = i | (i << 8) | (i << 16) | (i << 24);
}

double		exptable2[256 * 256];

void InitExpand2()
{
    int		i;
    int		j;
    // UNUSED unsigned	iexp, jexp;
    double* exp;
    union
    {
        double 		d;
        unsigned	u[2];
    } pixel;

    printf("building exptable2...\n");
    exp = exptable2;
    for (i = 0; i < 256; i++)
    {
        pixel.u[0] = i | (i << 8) | (i << 16) | (i << 24);
        for (j = 0; j < 256; j++)
        {
            pixel.u[1] = j | (j << 8) | (j << 16) | (j << 24);
            *exp++ = pixel.d;
        }
    }
    printf("done.\n");
}

int	inited;

void
Expand4
(unsigned* lineptr,
    double* xline)
{
    double	dpixel;
    unsigned	x;
    unsigned 	y;
    unsigned	fourpixels;
    unsigned	step;
    double* exp;

    exp = exptable2;
    if (!inited)
    {
        inited = 1;
        InitExpand2();
    }


    step = 3 * SCREENWIDTH / 2;

    y = SCREENHEIGHT - 1;
    do
    {
        x = SCREENWIDTH;

        do
        {
            fourpixels = lineptr[0];

            dpixel = *reinterpret_cast<double*>(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff0000) >> 13));
            xline[0] = dpixel;
            xline[160] = dpixel;
            xline[320] = dpixel;
            xline[480] = dpixel;

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff) << 3));
            xline[1] = dpixel;
            xline[161] = dpixel;
            xline[321] = dpixel;
            xline[481] = dpixel;

            fourpixels = lineptr[1];

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff0000) >> 13));
            xline[2] = dpixel;
            xline[162] = dpixel;
            xline[322] = dpixel;
            xline[482] = dpixel;

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff) << 3));
            xline[3] = dpixel;
            xline[163] = dpixel;
            xline[323] = dpixel;
            xline[483] = dpixel;

            fourpixels = lineptr[2];

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff0000) >> 13));
            xline[4] = dpixel;
            xline[164] = dpixel;
            xline[324] = dpixel;
            xline[484] = dpixel;

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff) << 3));
            xline[5] = dpixel;
            xline[165] = dpixel;
            xline[325] = dpixel;
            xline[485] = dpixel;

            fourpixels = lineptr[3];

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff0000) >> 13));
            xline[6] = dpixel;
            xline[166] = dpixel;
            xline[326] = dpixel;
            xline[486] = dpixel;

            dpixel = *(double*)(reinterpret_cast<intptr_t>(exp) + ((fourpixels & 0xffff) << 3));
            xline[7] = dpixel;
            xline[167] = dpixel;
            xline[327] = dpixel;
            xline[487] = dpixel;

            lineptr += 4;
            xline += 8;
        } while (x -= 16);
        xline += step;
    } while (y--);
}
