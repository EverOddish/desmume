#pragma once

#include "GPU.h"

void InitializeScreenReader(HWND parentWindow, HINSTANCE hInstance);

void UpdateScreenReader(const NDSDisplayInfo& displayInfo);
