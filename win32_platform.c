#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Windowsx.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "stroperation.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "error.h"
#include "maths.h"
#include "user_input.h"
#include "gl.h"
#include "token.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "globals.h"

float deltaTime = 0.0f;
static LARGE_INTEGER globalPerformanceFrequency;
static LARGE_INTEGER currentClock;

static LARGE_INTEGER get_clock()
{
	LARGE_INTEGER clock;
	QueryPerformanceCounter(&clock);
	return clock;
}

static void update_delta_time()
{
	LARGE_INTEGER newClock = get_clock();
	deltaTime = (float)(newClock.QuadPart - currentClock.QuadPart) / (float)globalPerformanceFrequency.QuadPart;
	currentClock = newClock;
}

static HWND window;

NO_RETURN void error(char *format, ...)
{
	char buffer[1024] = "ERROR : ";
	va_list arguments;
	va_start(arguments, format);
    vsprintf(buffer + 8, format, arguments);
	va_end(arguments);
	strcat(buffer, "\n");
	MessageBoxA(window, buffer, "Visual Novel Interpreter Error MessageBox", MB_ICONERROR);
	exit(EXIT_FAILURE);
}

void warning(char *format, ...)
{
	char buffer[1024] = "WARNING : ";
	va_list arguments;
	va_start(arguments, format);
    vsprintf(buffer + 8, format, arguments);
	va_end(arguments);
	strcat(buffer, "\n");
	MessageBoxA(window, buffer, "Visual Novel Interpreter Warning MessageBox", MB_ICONWARNING);
	exit(EXIT_FAILURE);
}

static void is_input_key_supported(InputKey inputKey)
{
	if (inputKey < 0 || inputKey > INPUT_KEY_COUNT)
	{
		error("testing unsupported input key : %d.", inputKey);
	}
}

static bool inputKeysBefore[INPUT_KEY_COUNT] = {0};
static bool inputKeysNow[INPUT_KEY_COUNT] = {0};

bool is_input_key_up(InputKey inputKey)
{
	is_input_key_supported(inputKey);
	return !inputKeysNow[inputKey];
}

bool is_input_key_down(InputKey inputKey)
{
	is_input_key_supported(inputKey);
	return inputKeysNow[inputKey];
}

bool is_input_key_released(InputKey inputKey)
{
	is_input_key_supported(inputKey);
	return inputKeysBefore[inputKey] && !inputKeysNow[inputKey];
}

bool is_input_key_pressed(InputKey inputKey)
{
	is_input_key_supported(inputKey);
	return !inputKeysBefore[inputKey] && inputKeysNow[inputKey];
}

void update_input_keys()
{
	memcpy(inputKeysBefore, inputKeysNow, sizeof (inputKeysBefore));
}

#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif

ivec2 mousePosition = {.x = 0, .y = 0};

static LRESULT WINAPI WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		DestroyWindow(hwnd);
		return 0;
	} else if (message == WM_DESTROY) {
		PostQuitMessage(0);
	} else if (message == WM_KEYUP || message == WM_KEYDOWN) {
		bool isInputKeyDown = (message == WM_KEYDOWN);
		if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
		{
			inputKeysNow[INPUT_KEY_NUMPAD_0 + wParam - VK_NUMPAD0] = isInputKeyDown;
		} else if (wParam >= 0x30 && wParam <= 0x39) {
			inputKeysNow[INPUT_KEY_0 + wParam - 0x30] = isInputKeyDown;
		} else if (wParam == VK_BACK) {
			inputKeysNow[INPUT_KEY_BACKSPACE] = isInputKeyDown;
		} else if (wParam == VK_TAB) {
			inputKeysNow[INPUT_KEY_TAB] = isInputKeyDown;
		} else if (wParam == VK_RETURN) {
			inputKeysNow[INPUT_KEY_ENTER] = isInputKeyDown;
		} else if (wParam == VK_SHIFT) {
			inputKeysNow[INPUT_KEY_SHIFT] = isInputKeyDown;
		} else if (wParam == VK_CONTROL) {
			inputKeysNow[INPUT_KEY_CTRL] = isInputKeyDown;
		} else if (wParam == VK_MENU) {
			inputKeysNow[INPUT_KEY_ALT] = isInputKeyDown;
		} else if (wParam == VK_CAPITAL) {
			inputKeysNow[INPUT_KEY_CAPS_LOCK] = isInputKeyDown;
		} else if (wParam == VK_ESCAPE) {
			inputKeysNow[INPUT_KEY_ESCAPE] = isInputKeyDown;
		} else if (wParam == VK_SPACE) {
			inputKeysNow[INPUT_KEY_SPACE] = isInputKeyDown;
		} else if (wParam == VK_PRIOR) {
			inputKeysNow[INPUT_KEY_PAGE_UP] = isInputKeyDown;
		} else if (wParam == VK_NEXT) {
			inputKeysNow[INPUT_KEY_PAGE_DOWN] = isInputKeyDown;
		} else if (wParam == VK_DELETE) {
			inputKeysNow[INPUT_KEY_DELETE] = isInputKeyDown;
		} else if (wParam == VK_UP) {
			inputKeysNow[INPUT_KEY_UP_ARROW] = isInputKeyDown;
		} else if (wParam == VK_DOWN) {
			inputKeysNow[INPUT_KEY_DOWN_ARROW] = isInputKeyDown;
		} else if (wParam == VK_LEFT) {
			inputKeysNow[INPUT_KEY_LEFT_ARROW] = isInputKeyDown;
		} else if (wParam == VK_RIGHT) {
			inputKeysNow[INPUT_KEY_RIGHT_ARROW] = isInputKeyDown;
		} else if (wParam == VK_F1) {
			inputKeysNow[INPUT_KEY_F1] = isInputKeyDown;
		} else if (wParam == VK_F2) {
			inputKeysNow[INPUT_KEY_F2] = isInputKeyDown;
		} else if (wParam == VK_F3) {
			inputKeysNow[INPUT_KEY_F3] = isInputKeyDown;
		} else if (wParam == VK_F4) {
			inputKeysNow[INPUT_KEY_F4] = isInputKeyDown;
		} else if (wParam == VK_F5) {
			inputKeysNow[INPUT_KEY_F5] = isInputKeyDown;
		} else if (wParam == VK_F6) {
			inputKeysNow[INPUT_KEY_F6] = isInputKeyDown;
		} else if (wParam == VK_F7) {
			inputKeysNow[INPUT_KEY_F7] = isInputKeyDown;
		} else if (wParam == VK_F8) {
			inputKeysNow[INPUT_KEY_F8] = isInputKeyDown;
		} else if (wParam == VK_F9) {
			inputKeysNow[INPUT_KEY_F9] = isInputKeyDown;
		} else if (wParam == VK_F10) {
			inputKeysNow[INPUT_KEY_F10] = isInputKeyDown;
		} else if (wParam == VK_F11) {
			inputKeysNow[INPUT_KEY_F11] = isInputKeyDown;
		} else if (wParam == VK_F12) {
			inputKeysNow[INPUT_KEY_F12] = isInputKeyDown;
		} else if (wParam >= 'A' && wParam <= 'Z') {
			inputKeysNow[INPUT_KEY_A + wParam - 'A'] = isInputKeyDown;
		}
	} else if (message == WM_LBUTTONDOWN) {
		inputKeysNow[INPUT_KEY_LEFT_MOUSE_BUTTON] = true;
	} else if (message == WM_RBUTTONDOWN) {
	   inputKeysNow[INPUT_KEY_RIGHT_MOUSE_BUTTON] = true;
	} else if (message == WM_MBUTTONDOWN) {
	   inputKeysNow[INPUT_KEY_MIDDLE_MOUSE_BUTTON] = true;
	} else if (message == WM_XBUTTONDOWN) {
		UINT button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
		    inputKeysNow[INPUT_KEY_SIDE_MOUSE_BUTTON_1] = true;
		}
		else if (button == XBUTTON2)
		{
		    inputKeysNow[INPUT_KEY_SIDE_MOUSE_BUTTON_2] = true;
		}
	} else if (message == WM_LBUTTONUP) {
		inputKeysNow[INPUT_KEY_LEFT_MOUSE_BUTTON] = false;
	} else if (message == WM_RBUTTONUP) {
	   inputKeysNow[INPUT_KEY_RIGHT_MOUSE_BUTTON] = false;
	} else if (message == WM_MBUTTONUP) {
	   inputKeysNow[INPUT_KEY_MIDDLE_MOUSE_BUTTON] = false;
	} else if (message == WM_XBUTTONUP) {
		UINT button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
		    inputKeysNow[INPUT_KEY_SIDE_MOUSE_BUTTON_1] = false;
		}
		else if (button == XBUTTON2)
		{
		    inputKeysNow[INPUT_KEY_SIDE_MOUSE_BUTTON_2] = false;
		}
	} else if (message == WM_MOUSEMOVE) {
		mousePosition.x = GET_X_LPARAM(lParam);
		mousePosition.y = GET_Y_LPARAM(lParam);
	} else {
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

#define GL_FUNCTION(ret, name, ...) ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	GL_LIST
#undef GL_FUNCTION

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001

#define WGL_LIST \
	WGL_FUNCTION(BOOL, wglChoosePixelFormatARB, HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats) \
	WGL_FUNCTION(HGLRC, wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const int* attribList) \
	WGL_FUNCTION(BOOL, wglSwapIntervalEXT, int interval)

#define WGL_FUNCTION(ret, name, ...) ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	WGL_LIST
#undef WGL_FUNCTION

ivec2 windowDimensions = {.x = 800, .y = 600};

mat4 projection;

static HCURSOR hCursor;

static HDC deviceContext;

void init_window()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASSA windowClass =
	{
		.lpfnWndProc = WindowProc,
		.hInstance = hInstance,
		.lpszClassName = "Visual Novel Interpreter Window Class",
		.style = CS_OWNDC
	};
	if (!RegisterClassA(&windowClass))
	{
		error("could not register window class.");
	}

	window = CreateWindowEx(0, windowClass.lpszClassName, NULL, ((WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) & ~WS_MAXIMIZEBOX) | WS_VISIBLE | WS_POPUP, 0, 0, windowDimensions.x, windowDimensions.y, NULL, NULL, hInstance, NULL);
	if (!window)
	{
		error("could not create window.");
	}

	deviceContext = GetDC(window);
	if (!deviceContext)
	{
		error("could not get a handle to a device context.");
	}

	SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetForegroundWindow(window);
	SetFocus(window);
	SetActiveWindow(window);
	EnableWindow(window, TRUE);

	hCursor = LoadCursor(NULL, IDC_ARROW);
	if (!hCursor)
	{
		error("could not load system arrow cursor.");
	}
    SetCursor(hCursor);

	PIXELFORMATDESCRIPTOR dummyPixelFormatDescriptor =
	{
		.nSize = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cDepthBits = 24,
		.cStencilBits = 8,
	};
	int dummyPixelFormat = ChoosePixelFormat(deviceContext, &dummyPixelFormatDescriptor);
	if (!dummyPixelFormat)
	{
		error("could not choose dummy pixel format.");
	}
	if (!SetPixelFormat(deviceContext, dummyPixelFormat, &dummyPixelFormatDescriptor))
	{
		error("could not set dummy pixel format.");
	}
	HGLRC dummyGlContext = wglCreateContext(deviceContext);
	if (!dummyGlContext)
	{
		error("could not create dummy OpenGL context.");
	}
	if (!wglMakeCurrent(deviceContext, dummyGlContext))
	{
		error("could not set dummy OpenGL context.");
	}

	{
		#define WGL_FUNCTION(ret, name, ...) \
			name = (void*)wglGetProcAddress(#name); \
			if (!name) \
			{ \
				error("can't load W OpenGL %s function.", #name); \
			}
		WGL_LIST
		#undef WGL_FUNCTION

		HMODULE dll = LoadLibraryA("opengl32.dll");
		if (!dll)
		{
			error("can't open OpenGL DLL."); \
		}

		#define GL_FUNCTION(ret, name, ...) \
			name = (void*)wglGetProcAddress(#name); \
			if (!name) \
			{ \
				name = (void*)GetProcAddress(dll, #name); \
				if (!name) \
				{ \
					error("can't load OpenGL %s function", #name); \
				} \
			}
		GL_LIST
		#undef GL_FUNCTION

		FreeLibrary(dll);
	}

	int pixelFormatAttribute[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0,
	};
	int pixelFormat;
	unsigned nbPixelFormat;
	wglChoosePixelFormatARB(deviceContext, pixelFormatAttribute, NULL, 1, &pixelFormat, &nbPixelFormat);
	if (!pixelFormat)
	{
		error("could not choose pixel format.");
	}

	int contextAttribute[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	HGLRC glContext = wglCreateContextAttribsARB(deviceContext, 0, contextAttribute);
	if (!glContext)
	{
		error("could not create OpenGL 3.3 context.");
	}
	if (!wglMakeCurrent(deviceContext, glContext))
	{
		error("could not set OpenGL 3.3 context.");
	}

	wglSwapIntervalEXT(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projection = mat4_ortho((float)(0), (float)(windowDimensions.x), (float)(windowDimensions.y), (float)(0));
}

void init_window_clock()
{
	QueryPerformanceFrequency(&globalPerformanceFrequency);
	currentClock = get_clock();
}

static MSG msg;
bool isWindowActive;

bool update_window()
{
	update_delta_time();
	update_input_keys();

	SetCursor(hCursor);

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	isWindowActive = (GetActiveWindow() == window);

	return true;
}

bool is_window_active()
{
	return GetActiveWindow() == window;
}

void swap_window_buffers()
{
	wglSwapLayerBuffers(deviceContext, WGL_SWAP_MAIN_PLANE);
}

void set_window_name(char *windowName)
{
	wchar_t *windowNameUtf16 = NULL;

	if (windowName)
	{
		int code;
		while (*windowName != '\0')
	    {
			code = utf8_decode(windowName);
			if (code == -1)
			{
				error("found incorrect utf-8 sequence in %s.", windowName);
			} else if (code > 0xFFFF) {
				error("unsupported character code %d.", code);
			}
			windowName += utf8Offset;
			buf_add(windowNameUtf16, code);
		}
		buf_add(windowNameUtf16, L'\0');
	}

	if (!SetWindowTextW(window, windowNameUtf16))
	{
		error("could not set the game name to the window.");
	}

	buf_free(windowNameUtf16);
}

void ask_window_to_close()
{
	PostMessageA(window, WM_CLOSE, 0, 0);
}

unsigned int shutdown_window()
{
	return msg.wParam;
}

#endif
