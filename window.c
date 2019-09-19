#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Windowsx.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "window.h"
#include "maths.h"
#include "globals.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "str.h"
#include "error.h"
#include "user_input.h"
#include "gl.h"

static HWND window;

NO_RETURN void error(const char *format, ...)
{
	char buffer[1024] = "ERROR : ";
	if (format)
	{
		va_list arguments;
		va_start(arguments, format);
		vsnprintf(buffer + 8, 1016, format, arguments);
		va_end(arguments);
		strncat(buffer, "\n", 2);
	} else {
		strncat(buffer, "no error message.\n", 19);
	}
	MessageBoxA(window, buffer, "Visual Novel Interpreter Error MessageBox", MB_ICONERROR);
	exit(EXIT_FAILURE);
}

void warning(const char *format, ...)
{
	char buffer[1024] = "WARNING : ";
	if (format)
	{
		va_list arguments;
		va_start(arguments, format);
		vsnprintf(buffer + 8, 1016, format, arguments);
		va_end(arguments);
		strncat(buffer, "\n", 2);
	} else {
		strncat(buffer, "no error message.\n", 19);
	}
	MessageBoxA(window, buffer, "Visual Novel Interpreter Warning MessageBox", MB_ICONWARNING);
}

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

ivec2 mousePosition = {0, 0};
ivec2 mouseOffset = {0, 0};

bool isWindowActive;

static DEVMODE originalScreenSettings = {0};

static DEVMODE newScreenSettings = {0};

static WindowMode currentWindowMode;

static void update_input_key(bool isInputKeyDown, WPARAM wParam, LPARAM lParam)
{
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
		int extended = (lParam & 0x01000000) != 0;
		if (!extended)
		{
			inputKeysNow[INPUT_KEY_CONTROL_LEFT] = isInputKeyDown;
		} else {
			inputKeysNow[INPUT_KEY_CONTROL_RIGHT] = isInputKeyDown;
		}
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
	} else if (wParam == VK_INSERT) {
		inputKeysNow[INPUT_KEY_INSER] = isInputKeyDown;
	} else if (wParam == VK_DELETE) {
		inputKeysNow[INPUT_KEY_DELETE] = isInputKeyDown;
	} else if (wParam == VK_HOME) {
		inputKeysNow[INPUT_KEY_BEGIN] = isInputKeyDown;
	} else if (wParam == VK_END) {
		inputKeysNow[INPUT_KEY_END] = isInputKeyDown;
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
	} else if (wParam == VK_ADD) {
		inputKeysNow[INPUT_KEY_ADD] = isInputKeyDown;
	} else if (wParam == VK_SUBTRACT) {
		inputKeysNow[INPUT_KEY_SUBTRACT] = isInputKeyDown;
	} else if (wParam == VK_MULTIPLY) {
		inputKeysNow[INPUT_KEY_MULTIPLY] = isInputKeyDown;
	} else if (wParam == VK_DIVIDE) {
		inputKeysNow[INPUT_KEY_DIVIDE] = isInputKeyDown;
	} else if (wParam == VK_DECIMAL) {
		inputKeysNow[INPUT_KEY_DOT] = isInputKeyDown;
	}
}

static LRESULT WINAPI WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		DestroyWindow(hwnd);
	} else if (message == WM_DESTROY) {
		PostQuitMessage(0);
	} else if (message == WM_SETFOCUS) {
		isWindowActive = true;
		if (currentWindowMode == WINDOW_MODE_FULLSCREEN)
		{
			ChangeDisplaySettings(&newScreenSettings, CDS_FULLSCREEN);
			glViewport(0, 0, windowDimensions.x, windowDimensions.y);
			MoveWindow(window, 0, 0, windowDimensions.x, windowDimensions.y, true);
		}
	} else if (message == WM_KILLFOCUS) {
		isWindowActive = false;
		if (currentWindowMode == WINDOW_MODE_FULLSCREEN)
		{
			ChangeDisplaySettings(&originalScreenSettings, CDS_RESET);
		}
		memset(inputKeysNow, 0, sizeof (inputKeysNow));
	} else if (message == WM_SYSCHAR) {
		/*printf("SYSCHAR : 0x%02X\n", wParam);
		fflush(stdout);*/
	} else if (message == WM_SYSKEYUP || message == WM_SYSKEYDOWN) {
		if (wParam == VK_F4)
		{
			ask_window_to_close();
			return 0;
		}
		bool isInputKeyDown = (message == WM_SYSKEYDOWN);
		/*if (isInputKeyDown)
		{
			printf("SYSKEYDOWN : 0x%02X\n", wParam);
		} else {
			printf("SYSKEYUP : 0x%02X\n", wParam);
		}
		fflush(stdout);*/
		update_input_key(isInputKeyDown, wParam, lParam);
	} else if (message == WM_KEYUP || message == WM_KEYDOWN) {
		bool isInputKeyDown = (message == WM_KEYDOWN);
		/*if (isInputKeyDown)
		{
			printf("KEYDOWN : 0x%02X\n", wParam);
		} else {
			printf("KEYUP : 0x%02X\n", wParam);
		}
		fflush(stdout);*/
		update_input_key(isInputKeyDown, wParam, lParam);
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
		unsigned int button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
			inputKeysNow[INPUT_KEY_SIDE_MOUSE_BUTTON_1] = false;
		}
		else if (button == XBUTTON2)
		{
			inputKeysNow[INPUT_KEY_SIDE_MOUSE_BUTTON_2] = false;
		}
	} else if (message == WM_MOUSEMOVE) {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		mouseOffset.x = x - mousePosition.x;
		mouseOffset.y = y - mousePosition.y;
		mousePosition.x = x;
		mousePosition.y = y;
	} else {
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

#define GL_FUNCTION(ret, name, ...) ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	GL_LIST
#undef GL_FUNCTION

#define WGL_DRAW_TO_WINDOW_ARB				0x2001
#define WGL_SUPPORT_OPENGL_ARB				0x2010
#define WGL_DOUBLE_BUFFER_ARB				0x2011
#define WGL_PIXEL_TYPE_ARB					0x2013
#define WGL_TYPE_RGBA_ARB					0x202B
#define WGL_COLOR_BITS_ARB					0x2014
#define WGL_DEPTH_BITS_ARB					0x2022
#define WGL_STENCIL_BITS_ARB				0x2023
#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB		0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB	0x00000001

#define WGL_LIST \
	WGL_FUNCTION(BOOL, wglChoosePixelFormatARB, HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats) \
	WGL_FUNCTION(HGLRC, wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const int* attribList) \
	WGL_FUNCTION(BOOL, wglSwapIntervalEXT, int interval)

#define WGL_FUNCTION(ret, name, ...) ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	WGL_LIST
#undef WGL_FUNCTION

ivec2 windowDimensions;
static ivec2 realWindowDimensions;
static ivec2 windowBorderDimensions;

mat4 projection;

static HCURSOR hCursor;

static HDC deviceContext;

void init_window(WindowMode windowMode, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASSA windowClass =
	{
		.lpfnWndProc = WindowProc,
		.hInstance = hInstance,
		.lpszClassName = "Visual Novel Interpreter Window Class",
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW
	};
	if (!RegisterClassA(&windowClass))
	{
		error("could not register window class.");
	}

	currentWindowMode = windowMode;
	originalScreenSettings.dmSize = sizeof (DEVMODE);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &originalScreenSettings);
	newScreenSettings.dmSize = sizeof (DEVMODE);

	windowDimensions.x = width;
	windowDimensions.y = height;

	RECT rect = {0, 0, width, height};
	AdjustWindowRect(&rect, (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX) | WS_VISIBLE | WS_POPUP, true);
	//printf("%d - %d - %d - %d\n", rect.left, rect.top, rect.right, rect.bottom);
	windowBorderDimensions.x = (rect.right - rect.left) - width;
	windowBorderDimensions.y = (rect.bottom - rect.top) - height - 20;
	//printf("%d - %d\n", windowBorderDimensions.x, windowBorderDimensions.y);

	if (windowMode == WINDOW_MODE_WINDOWED)
	{
		realWindowDimensions.x = width + windowBorderDimensions.x;
		realWindowDimensions.y = height + windowBorderDimensions.y;
		window = CreateWindow(windowClass.lpszClassName, NULL, (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX) | WS_VISIBLE | WS_POPUP, (originalScreenSettings.dmPelsWidth - realWindowDimensions.x) / 2, (originalScreenSettings.dmPelsHeight - realWindowDimensions.y) / 2, realWindowDimensions.x, realWindowDimensions.y, NULL, NULL, hInstance, NULL);
	} else if (windowMode == WINDOW_MODE_BORDERLESS) {
		realWindowDimensions.x = width;
		realWindowDimensions.y = height;
		window = CreateWindow(windowClass.lpszClassName, NULL, WS_VISIBLE | WS_POPUP, (originalScreenSettings.dmPelsWidth - realWindowDimensions.x) / 2, (originalScreenSettings.dmPelsHeight - realWindowDimensions.y) / 2, realWindowDimensions.x, realWindowDimensions.y, NULL, NULL, hInstance, NULL);
	} else if (windowMode == WINDOW_MODE_FULLSCREEN) {
		realWindowDimensions.x = width;
		realWindowDimensions.y = height;
		window = CreateWindow(windowClass.lpszClassName, NULL, WS_VISIBLE | WS_POPUP, 0, 0, realWindowDimensions.x, realWindowDimensions.y, NULL, NULL, hInstance, NULL);
		DEVMODE screenSettings = {0};
		screenSettings.dmSize = sizeof (screenSettings);
		screenSettings.dmPelsWidth = realWindowDimensions.x;
		screenSettings.dmPelsHeight = realWindowDimensions.y;
		screenSettings.dmBitsPerPel = 32;
		screenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);
	}

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
	EnableWindow(window, true);
	isWindowActive = true;

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
				error("could not load WGL function \"%s\".", #name); \
			}
		WGL_LIST
		#undef WGL_FUNCTION

		HMODULE OpenGLLibrary = LoadLibraryA("opengl32.dll");
		if (!OpenGLLibrary)
		{
			error("could not load OpenGL library."); \
		}

		#define GL_FUNCTION(ret, name, ...) \
			name = (void*)wglGetProcAddress(#name); \
			if (!name) \
			{ \
				name = (void*)GetProcAddress(OpenGLLibrary, #name); \
				if (!name) \
				{ \
					error("could not load OpenGL function \"%s\".", #name); \
				} \
			}
		GL_LIST
		#undef GL_FUNCTION

		FreeLibrary(OpenGLLibrary);
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

	QueryPerformanceFrequency(&globalPerformanceFrequency);
	currentClock = get_clock();

	projection = mat4_ortho(0.0f, (float)(windowDimensions.x), (float)(windowDimensions.y), 0.0f);
}

static MSG msg;

bool update_window()
{
	update_delta_time();
	update_input_keys();

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

void swap_window_buffers()
{
	wglSwapLayerBuffers(deviceContext, WGL_SWAP_MAIN_PLANE);
}

void ask_window_to_close()
{
	PostMessageA(window, WM_CLOSE, 0, 0);
}

unsigned int get_window_shutdown_return_code()
{
	return msg.wParam;
}

void set_window_name(const char *windowName)
{
	buf(wchar_t) windowNameUtf16 = NULL;

	if (windowName)
	{
		buf(int) codes = utf8_decode(windowName);
		windowNameUtf16 = codepoint_to_utf16(codes);
		buf_free(codes);
		buf_add(windowNameUtf16, L'\0');
	}

	LONG_PTR originalWndProc = GetWindowLongPtrW(window, GWLP_WNDPROC);
	SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR) DefWindowProcW);
	if (!SetWindowTextW(window, windowNameUtf16))
	{
		error("could not set the game name to the window.");
	}
	SetWindowLongPtrW(window, GWLP_WNDPROC, originalWndProc);

	buf_free(windowNameUtf16);
}

void set_window_vsync(bool sync)
{
	if (sync)
	{
		wglSwapIntervalEXT(-1);
	} else {
		wglSwapIntervalEXT(0);
	}
}

void set_window_clear_color(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void resize_window(int width, int height)
{
	windowDimensions.x = width;
	windowDimensions.y = height;
	realWindowDimensions.x = width;
	realWindowDimensions.y = height;
	if (currentWindowMode == WINDOW_MODE_WINDOWED)
	{
		windowDimensions.x -= windowBorderDimensions.x;
		windowDimensions.y -= windowBorderDimensions.y;
	}

	if (currentWindowMode == WINDOW_MODE_FULLSCREEN)
	{
		MoveWindow(window, 0, 0, realWindowDimensions.x, realWindowDimensions.y, true);
	} else {
		MoveWindow(window, (originalScreenSettings.dmPelsWidth - realWindowDimensions.x) / 2, (originalScreenSettings.dmPelsHeight - realWindowDimensions.y) / 2, realWindowDimensions.x, realWindowDimensions.y, true);
	}

	glViewport(0, 0, windowDimensions.x, windowDimensions.y);
	projection = mat4_ortho(0.0f, (float)(windowDimensions.x), (float)(windowDimensions.y), 0.0f);
}

void set_window_mode(WindowMode windowMode)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	/*printf("%d - %d - %d - %d\n", viewport[0], viewport[1], viewport[2], viewport[3]);
	printf("%d - %d\n", realWindowDimensions.x, realWindowDimensions.y);
	printf("%d - %d\n", windowDimensions.x, windowDimensions.y);*/

	if (windowMode == currentWindowMode)
	{
		return;
	}

	if (windowMode == WINDOW_MODE_WINDOWED)
	{
		realWindowDimensions.x += windowBorderDimensions.x;
		realWindowDimensions.y += windowBorderDimensions.y;
		SetWindowLongPtr(window, GWL_STYLE, (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX) | WS_VISIBLE | WS_POPUP);
		ChangeDisplaySettings(&originalScreenSettings, CDS_RESET);
	} else if (windowMode == WINDOW_MODE_BORDERLESS) {
		if (currentWindowMode == WINDOW_MODE_WINDOWED)
		{
			realWindowDimensions.x -= windowBorderDimensions.x;
			realWindowDimensions.y -= windowBorderDimensions.y;
		}
		SetWindowLongPtr(window, GWL_STYLE, WS_VISIBLE | WS_POPUP);
		ChangeDisplaySettings(&originalScreenSettings, CDS_RESET);
	} else if (windowMode == WINDOW_MODE_FULLSCREEN) {
		if (currentWindowMode == WINDOW_MODE_WINDOWED)
		{
			realWindowDimensions.x -= windowBorderDimensions.x;
			realWindowDimensions.y -= windowBorderDimensions.y;
		}
		SetWindowLongPtr(window, GWL_STYLE, WS_VISIBLE | WS_POPUP);
		newScreenSettings.dmPelsWidth = realWindowDimensions.x;
		newScreenSettings.dmPelsHeight = realWindowDimensions.y;
		newScreenSettings.dmBitsPerPel = 32;
		newScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		ChangeDisplaySettings(&newScreenSettings, CDS_FULLSCREEN);
	}

	currentWindowMode = windowMode;
	resize_window(realWindowDimensions.x, realWindowDimensions.y);
}
