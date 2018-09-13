#include <Windows.h>
#include <Windowsx.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "globals.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "gl.h"
#include "graphics.h"
#include "lex.h"
#include "interpret.h"

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

char *file_to_string(char *filePath)
{
	FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        error("could not open %s.", filePath);
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize == -1L)
    {
        error("could not get size of %s.", filePath);
    }
    fileSize++;  // For '\0'
    rewind(file);

    char *fileString = xmalloc(sizeof (*fileString) * fileSize);
    size_t result = fread(fileString, sizeof (char), fileSize - 1, file);
    if (result != (unsigned long)fileSize - 1)
    {
        error("could not read %s.", filePath);
    }
    fileString[fileSize - 1] = '\0';

	fclose(file);

	return fileString;
}

bool strmatch(char *a, char *b)
{
	return !strcmp(a, b);
}

ivec2 windowDimensions = {.x = 1600, .y = 900};

mat4 projection;

static bool running = true;

static LARGE_INTEGER globalPerformanceFrequency;
static LARGE_INTEGER currentClock;
float deltaTime = 0.0f;

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

static MSG msg;

ivec2 mousePosition = {.x = 0, .y = 0};
static bool inputKeysBefore[INPUT_KEY_COUNT] = {0};
static bool inputKeysNow[INPUT_KEY_COUNT] = {0};

static void is_input_key_supported(InputKey inputKey)
{
	if (inputKey < 0 || inputKey > INPUT_KEY_COUNT)
	{
		error("testing unsupported input key : %d.", inputKey);
	}
}

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

static LRESULT WINAPI WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		return DefWindowProc(hwnd, message, wParam, lParam);
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
	WGL_FUNCTION(HGLRC, wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const int* attribList)

#define WGL_FUNCTION(ret, name, ...) ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	WGL_LIST
#undef WGL_FUNCTION

char *nextDialog = NULL;
bool gameEnd = false;
char **variablesNames = NULL;
double *variablesValues = NULL;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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

	window = CreateWindowEx(0, windowClass.lpszClassName, "Visual Novel Interpreter Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 0, 0, windowDimensions.x, windowDimensions.y, NULL, NULL, hInstance, NULL);
	if (!window)
	{
		error("could not create window.");
	}

	HDC deviceContext = GetDC(window);
	if (!deviceContext)
	{
		error("could not get a handle to a device context.");
	}

	if (GetConsoleWindow())
	{
		if (!FreeConsole())
		{
			error("could not detach the process from the launch console.");
		}
	}
	if (!AllocConsole())
	{
		error("could not allocate a console.");
	}
	if (!freopen("conin$", "r", stdin))
	{
		error("could not redirect conin into stdin.");
	}
	if (!freopen("conout$", "w", stdout))
	{
		error("could not redirect conout into stdout.");
	}
	if (!freopen("conout$", "w", stderr))
	{
		error("could not redirect conout into stderr.");
	}
	HWND consoleHandle = GetConsoleWindow();
	if (!consoleHandle)
	{
		error("could not get the allocated console handle.");
	}
	if (!MoveWindow(consoleHandle, 0, 0, 800, 600, TRUE))
	{
		error("could not move console to initial position.");
	}

	SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetForegroundWindow(window);
	SetFocus(window);
	SetActiveWindow(window);
	EnableWindow(window, TRUE);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projection = mat4_ortho((float)(0), (float)(windowDimensions.x), (float)(windowDimensions.y), (float)(0));

	init_graphics();
	init_dialog_ui();

	Token *tokens = lex("Dialogs/dialog.dlg");
	Dialog *dialog = create_dialog("Dialogs/dialog.dlg", tokens);

	/*vec2 position = {.x = 0.0f, .y = 0.0f};
	vec3 color = {.x = 0.3f, .y = 0.5f, .z = 0.8f};
	Text text = create_text(position, TEXT_SIZE_NORMAL, "éàüî", "Fonts/arial.ttf", color);*/

	QueryPerformanceFrequency(&globalPerformanceFrequency);

	currentClock = get_clock();

	while (running)
	{
		update_delta_time();
		update_input_keys();

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		//add_text_to_draw_list(text, DRAW_LAYER_FOREGROUND);

		if (!gameEnd)
		{
			interpret(dialog);
			if (nextDialog)
			{
				for (unsigned int index = 0; index < buf_len(tokens); index++)
			    {
			        free_token(tokens[index]);
			    }
			    buf_free(tokens);
				free_dialog(dialog);
				tokens = lex(nextDialog);
				dialog = create_dialog(nextDialog, tokens);
				nextDialog = NULL;
			}
		} else {
			PostMessageA(window, WM_CLOSE, 0, 0);
		}

		draw_all();

		wglSwapLayerBuffers(deviceContext, WGL_SWAP_MAIN_PLANE);
	}
	return msg.wParam;
}
