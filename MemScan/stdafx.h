// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <Windows.h>
#include <string>
#include <strsafe.h>

std::wstring GetErrorString(PCTSTR pszFunction);
std::string format_string(const std::string fmt, ...);
