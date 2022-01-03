#include <string>
#include <iostream>

#include <Windows.h>

#include "screenReader.h"
#include "chatot_lib.h"

#define ID_EDITCHILD 100

static uint64_t g_stepCounter = 0;
static std::string g_lastText;
static std::string g_textBuffer;
static HWND g_textWindow = NULL;
static HWND g_editControl = NULL;

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
	ChatotLib_Initialize();

	const char CLASS_NAME[] = "DeSmuME Screen Reader Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	g_textWindow = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		"DeSmuME Screen Reader",    // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		parentWindow,       // Parent window    
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
}

void UpdateScreenReader(const NDSDisplayInfo& displayInfo)
{
	// Perform OCR every 5 steps
	if (0 == g_stepCounter % 5)
	{
		std::string text;
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

		for (int i = 0; i < 2; i++)
		{
			if (displayInfo.didPerformCustomRender[i])
			{
				ChatotLib_GetTextFromScreen(displayInfo.customBuffer[i], displayInfo.customWidth, displayInfo.customHeight, format, text);
			}
			else
			{
				ChatotLib_GetTextFromScreen(displayInfo.nativeBuffer16[i], GPU_FRAMEBUFFER_NATIVE_WIDTH, GPU_FRAMEBUFFER_NATIVE_HEIGHT, format, text);
			}

			if (text.length() > 0)
			{
				//std::cout << "Original text: " << text << std::endl;

				std::string outText;
				ChatotLib_CorrectText(text, outText);

				//std::cout << "Correctd text: " << outText << std::endl;

				if (g_lastText == outText)
				{
					outText = "";
				}
				else
				{
					int i = 0;
					for (i = 0; i < outText.length(); i++)
					{
						if (g_lastText[i] != outText[i])
						{
							outText = outText.substr(i);
							break;
						}
					}
				}

				if (outText.length() > 0)
				{
					g_textBuffer += outText;
					g_lastText = outText;

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
		}
	}

	g_stepCounter++;
}