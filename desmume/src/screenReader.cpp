#include <string>
#include <iostream>

#include "screenReader.h"
#include "chatot_lib.h"

void UpdateScreenReader(const NDSDisplayInfo& displayInfo)
{
	std::string text;
	CLColourFormat format;

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

	ChatotLib_GetTextFromScreen(displayInfo.masterCustomBuffer, displayInfo.customWidth, displayInfo.customHeight, format, text);
}