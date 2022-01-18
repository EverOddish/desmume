#include <stdlib.h>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>

#include <Windows.h>

#include "screenReader.h"
#include "chatot_lib.h"

#define ID_EDITCHILD 100

typedef struct
{
	void* buffer;
	uint32_t width;
	uint32_t height;
	CLColourFormat format;
} UpdateInfo;

void DoUpdateWork();

using namespace std::chrono_literals;

static std::mutex g_mutex;
static std::string g_lastText;
static std::string g_textBuffer;
static HWND g_textWindow = NULL;
static HWND g_editControl = NULL;
static UpdateInfo* g_updateInfo[2] = { 0 };

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		g_editControl = CreateWindowEx(0,
			"EDIT",
			"",
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,
			hwnd,
			(HMENU)ID_EDITCHILD,
			(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
			NULL);		// extra data

		if (g_editControl != NULL)
		{
			DWORD width;
			DWORD height;
			RECT rect;

			GetWindowRect(hwnd, &rect);
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;

			MoveWindow(g_editControl,
				0, 0,                  // starting x- and y-coordinates 
				width,        // width of client area 
				height,        // height of client area 
				TRUE);                 // repaint window 
		}
		else
		{
			std::cout << "Failed to create edit control!" << std::endl;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void InitializeScreenReader(HWND parentWindow, HINSTANCE hInstance)
{
	const char CLASS_NAME[] = "DeSmuME Screen Reader Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	g_textWindow = CreateWindowEx(
		WS_EX_APPWINDOW,            // Optional window styles.
		CLASS_NAME,                 // Window class
		"DeSmuME Screen Reader",    // Window text
		WS_OVERLAPPEDWINDOW,        // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (g_textWindow != NULL)
	{
		ShowWindow(g_textWindow, SW_NORMAL);
	}
	else
	{
		std::cout << "Failed to create window!" << std::endl;
	}

	std::thread t(DoUpdateWork);
	t.detach();
}

UpdateInfo* MakeUpdateInfo(void* buffer, uint32_t width, uint32_t height, uint32_t bytesPerPixel, CLColourFormat format)
{
	UpdateInfo* info = (UpdateInfo*) malloc(sizeof(UpdateInfo));

	if (info)
	{
		uint32_t bufferSize = width * height * bytesPerPixel;

		info->buffer = malloc(bufferSize);
		if (info->buffer)
		{
			memcpy(info->buffer, buffer, bufferSize);
		}
		info->width = width;
		info->height = height;
		info->format = format;
	}

	return info;
}

void FreeUpdateInfo(UpdateInfo* info)
{
	if (info)
	{
		if (info->buffer)
		{
			free(info->buffer);
		}

		free(info);
	}
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void DoUpdateWork()
{
	ChatotLib_Initialize();

	while (1)
	{
		g_mutex.lock();

		for (int i = 0; i < 2; i++)
		{
			if (g_updateInfo[ i ])
			{
				std::string text;

				ChatotLib_GetTextFromScreen(g_updateInfo[ i ]->buffer,
											g_updateInfo[ i ]->width,
											g_updateInfo[ i ]->height,
											g_updateInfo[ i ]->format,
											text);
				replaceAll(text, "\n", "");

				while (0 == text.find(" "))
				{
					text.replace(text.begin(), text.begin() + 1, "");
				}

				if (text.length() > 0)
				{
					std::string newText;

					//std::cout << "Original text: " << text << std::endl;

					//std::string outText;
					//ChatotLib_CorrectText(text, outText);

					//std::cout << "Correctd text: " << outText << std::endl;

					if (g_lastText == text)
					{
						newText = "";
					}
					else
					{
						int i = 0;
						for (i = 0; i < text.length(); i++)
						{
							if (tolower(g_lastText[i]) != tolower(text[i]))
							{
								newText = text.substr(i);
								break;
							}
						}
					}

					if (newText.length() > 0)
					{
						//std::cout << "New text: " << newText << std::endl;

						g_textBuffer += newText;
						g_lastText = text;

						if (g_textBuffer.length() > 500)
						{
							size_t cursor = 0;

							// Delete first ten words
							for (int i = 0; i < 10 && cursor != std::string::npos; i++)
							{
								cursor = g_textBuffer.find(" ", cursor);
							}

							g_textBuffer = g_textBuffer.substr(cursor);
						}

						//std::cout << "Text buffer: " << g_textBuffer << std::endl;

						SendMessage(g_editControl, WM_SETTEXT, 0, (LPARAM)g_textBuffer.c_str());
					}
				}

				FreeUpdateInfo(g_updateInfo[ i ]);
				g_updateInfo[ i ] = NULL;
			}
		}

		g_mutex.unlock();

		// Give a chance for the next frame
		std::this_thread::sleep_for(50ms);
	}
}

void UpdateScreenReader(const NDSDisplayInfo& displayInfo)
{
	/*
	CLColourFormat format = BGR555;

	switch (displayInfo.colorFormat)
	{
	case NDSColorFormat_BGR555_Rev:
		format = BGR555;
		break;
	case NDSColorFormat_BGR666_Rev:
		format = BGR666;
		break;
	case NDSColorFormat_BGR888_Rev:
		format = BGR888;
		break;
	}
	*/

	if (g_mutex.try_lock())
	{
		for (int i = 0; i < 2; i++)
		{
			if (displayInfo.didPerformCustomRender[i])
			{
				g_updateInfo[ i ] = MakeUpdateInfo(displayInfo.customBuffer[i], displayInfo.customWidth, displayInfo.customHeight, displayInfo.pixelBytes, BGR888);
			}
			else
			{
				g_updateInfo[ i ] = MakeUpdateInfo(displayInfo.nativeBuffer16[i], GPU_FRAMEBUFFER_NATIVE_WIDTH, GPU_FRAMEBUFFER_NATIVE_HEIGHT, displayInfo.pixelBytes, BGR888);
			}
		}

		g_mutex.unlock();
	}
}