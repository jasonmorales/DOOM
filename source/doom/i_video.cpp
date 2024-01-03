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
//	DOOM graphics stuff for Windows
//
//-----------------------------------------------------------------------------
import std;
import nstd;

import config;
import platform;

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "d_main.h"
#include "m_bbox.h"
#include "st_stuff.h"
#include "doomdef.h"
#include "z_zone.h"

#include <GL/wglew.h>
#include <gl/gl.h>

#undef CreateWindow
#undef RegisterClass

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

template<typename ...Ts>
class gl_vertex_attributes
{
public:
    static constexpr const GLsizei size = []{
        if constexpr (sizeof...(Ts) == 0) return 0;
        else return static_cast<GLsizei>((sizeof(Ts)+...));
    }();

    static void add()
    {
        GLuint index = 0;
        intptr_t offset = 0;

        _add<Ts...>(index, offset);
    }

private:
    template<typename V, size_t R = 0>
    static constexpr GLint count()
    {
        if constexpr (!std::is_array_v<V>)
            return 1;
        else if constexpr (R < std::rank_v<V> - 1)
            return std::extent_v<V, R> * count<V, R + 1>();
        else
            return std::extent_v<V, R>;
    }

    template<typename T, typename ...Ts>
    static void _add(GLuint& index, intptr_t& offset)
    {
        glEnableVertexAttribArray(index);
        if constexpr (nstd::is_integral<T>)
            glVertexAttribIPointer(index, count<T>(), gl_type<std::remove_all_extents_t<T>>, size, reinterpret_cast<GLvoid*>(offset));
        else
            glVertexAttribPointer(index, count<T>(), gl_type<std::remove_all_extents_t<T>>, GL_FALSE, size, reinterpret_cast<GLvoid*>(offset));

        offset += sizeof(T);
        index += 1;
        if constexpr (sizeof...(Ts) != 0)
            _add<Ts...>(index, offset);
    }
};

void I_ShutdownGraphics()
{
}

static int	lastmousex = 0;
static int	lastmousey = 0;
bool mousemoved = false;

void Video::FinishUpdate()
{
    static time_t lasttic = 0;

    // draws little dots on the bottom of the screen
    if (doom->IsDevMode())
    {
        auto i = I_GetTime();
        auto tics = i - lasttic;
        lasttic = i;
        if (tics > 20) tics = 20;

        for (i = 0; i < tics * 2; i += 2)
            screenBuffer[((windowHeight/screenMultiply- 1)*screenTextureSize)  + i] = 0xff'ff'ff'ff;
        for (; i < 20 * 2; i += 2)
            screenBuffer[((windowHeight/screenMultiply- 1)*screenTextureSize)  + i] = 0xff'00'00'00;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);

    glUseProgram(screenShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    for (uint32 y = 0; y < windowHeight / screenMultiply; ++y)
    for (uint32 x = 0; x < windowWidth / screenMultiply; ++x)
    {
        screenBuffer[y * screenTextureSize + x] = palette[screens[0][y * SCREENWIDTH + x]];
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenTextureSize, screenTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);

    //glDrawBuffer(GL_COLOR_ATTACHMENT2);
    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (window)
        window->SwapBuffers();
}

void Video::SetPalette(const byte* inPalette)
{
    auto* p = inPalette;
    for (int32 n = 0; n < 256; ++n, p += 3)
    {
        palette[n] = 0xff'00'00'00 | (*(p + 2) << 16) | (*(p + 1) << 8) | (*(p + 0) << 0);
    }
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
    std::stringstream out;
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

    /*
    #define GL_DEBUG_TYPE_ERROR 0x824C
    #define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
    #define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
    #define GL_DEBUG_TYPE_PORTABILITY 0x824F
    #define GL_DEBUG_TYPE_PERFORMANCE 0x8250
    #define GL_DEBUG_TYPE_OTHER 0x8251
    #define GL_DEBUG_TYPE_MARKER 0x8268

    #define GL_DEBUG_SEVERITY_HIGH 0x9146
    #define GL_DEBUG_SEVERITY_MEDIUM 0x9147
    #define GL_DEBUG_SEVERITY_LOW 0x9148
    #define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
    */

    std::cerr << message << "\n";

    if ((type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
        && (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW))
        DebugBreak();
}

void Video::Init()
{
    if (isInitialized)
        return;

    isInitialized = true;

    byte* base = new byte[SCREENWIDTH * SCREENHEIGHT * 4];
    for (int32 i = 0; i < 4; ++i)
        screens[i] = base + i * SCREENWIDTH * SCREENHEIGHT;
    screens[4] = (byte*)Z_Malloc(ST_WIDTH * ST_HEIGHT, PU_STATIC, 0);

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

    WindowClassDefinition windowClass{};
    platform::Window::RegisterClass(windowClass);

    WindowDefinition windowDefinition{
        .title = L"Doom",
        .class_id = windowClass.id,
        .width = nstd::size_cast<int32>(windowWidth),
        .height = nstd::size_cast<int32>(windowHeight),
        .acceptFileDrops = false,
        .isResizeable = false,
    };
    window = platform::CreateWindow(windowDefinition);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLErrorCallback, 0);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    glFrontFace(GL_CCW);

    glDisable(GL_MULTISAMPLE);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    screenTextureSize = 512u; //std::min(std::bit_ceil(std::max(windowWidth, windowHeight)), 4096u);
    screenBuffer = new uint32[screenTextureSize * screenTextureSize];
    for (uint32 n = 0; n < screenTextureSize * screenTextureSize; ++n)
    {
        uint32 x = n % screenTextureSize;
        uint32 y = n / screenTextureSize;

        screenBuffer[n] =
            (y==(windowHeight/screenMultiply)-1) ? 0xff'00'ff'ff :
            (x==(windowWidth/screenMultiply)-1) ? 0xff'ff'00'ff :
            (y%20==0) ? 0xff'00'ff'00 :
            (x%20==0) ? 0xff'ff'00'00 :
            0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenTextureSize, screenTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    screenShader = LoadShader("screen");

    float s = 1.f - ((screenTextureSize - windowWidth / screenMultiply) / static_cast<float>(screenTextureSize));
    float t = 1.f - ((screenTextureSize - windowHeight / screenMultiply) / static_cast<float>(screenTextureSize));
    float screenVertices[] =
    {
        -1.f,  -1.f,  0.f, t,
        -1.f, 1.f,  0.f, 0.f,
        1.f, 1.f,  s, 0.f,

        -1.f,  -1.f,  0.f, t,
        1.f, 1.f,  s, 0.f,
        1.f,  -1.f,  s, t,
    };

    glGenVertexArrays(1, &screenVAO);
    glBindVertexArray(screenVAO);

    glGenBuffers(1, &screenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

    gl_vertex_attributes<float[2], float[2]>::add();

    if (window)
        window->Show();
}

void Video::StartFrame()
{
    glClearColor(0.f, 1.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Video::StartTick()
{
    DeliverSystemMessages();
}

void Video::DeliverSystemMessages()
{
    // TODO: This could be in a thread
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        // Check for a WM_QUIT message
        auto result = GetMessage(&msg, NULL, 0, 0);
        if (result == -1)
            continue;

        if (result == 0)
            I_Quit();

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Masks a column based masked pic to the screen. 
void Video::DrawPatch(int32 x, int32 y, int32 screen, const patch_t* patch)
{
    y -= patch->topoffset;
    x -= patch->leftoffset;
#ifdef RANGECHECK 
    if (x<0
        || x + (patch->width) >SCREENWIDTH
        || y<0
        || y + (patch->height)>SCREENHEIGHT
        || screen > 4)
    {
        std::cerr << std::format("Patch at {},{} exceeds LFB\n", x, y);
        // No I_Error abort - what is up with TNT.WAD?
        std::cerr << "Video::DrawPatch: bad patch (ignored)\n";
        return;
    }
#endif 

    auto* desttop = screens[screen] + y * SCREENWIDTH + x;
    auto w = patch->width;
    for (int32 col = 0; col < w; x++, col++, desttop++)
    {
        auto* column = (column_t*)((byte*)patch + (patch->columnofs[col]));

        // step through the posts in a column 
        while (column->topdelta != 0xff)
        {
            auto* source = (byte*)column + 3;
            auto* dest = desttop + column->topdelta * SCREENWIDTH;
            auto count = column->length;

            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
            column = (column_t*)((byte*)column + column->length + 4);
        }
    }
}

byte* Video::CopyScreen(int32 dest) const
{
    assert(dest > 0 && dest < std::size(screens));
    memcpy(screens[dest], screens[0], SCREENWIDTH * SCREENHEIGHT);
    return screens[dest];
}

GLuint Video::LoadShader(string_view name)
{
    std::cout << "Loading shader: " << name << "\n";

    auto program = glCreateProgram();

    string shaderName{name};

    std::filesystem::path path("data/shaders/");
    path /= shaderName + ".glsl";

    auto addShader = [&path, &shaderName, program](const string& ext, GLenum type, bool required)
    {
        std::ifstream file(path.replace_filename(shaderName + ext + ".glsl"), std::ios_base::binary | std::ios_base::in);
        string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (data.empty())
        {
            if (required)
                std::cerr << "Missing required shader: " << shaderName + ext << "\n";
            return;
        }

        auto shader = glCreateShader(type);
        auto* vdata = data.c_str();
        glShaderSource(shader, 1, &vdata, nullptr);
        glCompileShader(shader);
        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            char buffer[4096] = {0};
            int32 length = 0;
            glGetShaderInfoLog(shader, sizeof(buffer), &length, buffer);
            std::cerr << "Error compiling shader " << shaderName + ext << ": " << buffer << "\n";
        }
        glAttachShader(program, shader);
    };

    // vertex
    addShader("_v", GL_VERTEX_SHADER, true);
    addShader("_g", GL_GEOMETRY_SHADER, false);
    addShader("_f", GL_FRAGMENT_SHADER, true);

    glLinkProgram(program);

    return program;
}
