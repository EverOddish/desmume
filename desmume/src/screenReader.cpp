#include <string>
#include <iostream>

#include "screenReader.h"
#include "chatot_lib.h"

void InitializeScreenReader()
{
	ChatotLib_Initialize();
}

void UpdateScreenReader(const NDSDisplayInfo& displayInfo)
{
	std::string text;
	CLColourFormat format = BGR555;

	switch(displayInfo.colorFormat)
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
			std::cout << text << std::endl;
		}
	}
}