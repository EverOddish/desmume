#include <string>
#include <iostream>

#include "screenReader.h"
#include "chatot_lib.h"

static uint64_t g_stepCounter = 0;
static std::string g_lastText;
static std::string g_textBuffer;

void InitializeScreenReader()
{
	ChatotLib_Initialize();
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

					std::cout << "Text buffer: " << g_textBuffer << std::endl;
				}
			}
		}
	}

	g_stepCounter++;
}