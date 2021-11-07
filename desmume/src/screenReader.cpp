#include <string>

#include "screenReader.h"
#include "chatot_lib.h"

void UpdateScreenReader( void* screenBuffer, u32 rows, u32 columns)
{
	std::string text;

	ChatotLib_GetTextFromScreen(screenBuffer, rows, columns, text);
}