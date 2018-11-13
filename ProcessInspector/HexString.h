// 08/26/2008

#pragma once

#include <vector>

USHORT HexStringBytes (LPCTSTR pszData);
int HexStringToArray (LPCTSTR pszData, UCHAR data[], USHORT length);
int HexStringToArray (LPCTSTR pszData, USHORT data[], USHORT length);
int HexStringToArray (LPCTSTR pszData, std::vector<UCHAR>& data);
CString HexArrayToString (UCHAR data[], int length);
CString HexFillString (UCHAR cFill, int length);



