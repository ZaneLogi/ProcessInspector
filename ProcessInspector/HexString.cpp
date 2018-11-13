#include "stdafx.h"
#include "HexString.h"

USHORT HexStringBytes (LPCTSTR pszData)
{
	int i = 0;
	for (i = 0;; ++i) {
		while (*pszData && !::_istxdigit(*pszData)) ++pszData;
		if (*pszData == 0) break;
		LPTSTR pszEnd = 0;
		ULONG val = ::_tcstoul(pszData, &pszEnd, 16);
		pszData = pszEnd;
		if (val > 0xFF) break;
	}
	return i;
}

int HexStringToArray (LPCTSTR pszData, UCHAR data[], USHORT length)
{
	int i;
	for (i = 0; i < length; ++i) {
		while (*pszData && !::_istxdigit(*pszData)) ++pszData;
		if (*pszData == 0) break;
		LPTSTR pszEnd = 0;
		ULONG val = ::_tcstoul(pszData, &pszEnd, 16);
		pszData = pszEnd;
		if (val > 0xFF) break;
		data[i] = (BYTE)val;
	}
	return i;
}

int HexStringToArray (LPCTSTR pszData, USHORT data[], USHORT length)
{
	int i;
	for (i = 0; i < length; ++i) {
		while (*pszData && !::_istxdigit(*pszData)) ++pszData;
		if (*pszData == 0) break;
		LPTSTR pszEnd = 0;
		ULONG val = ::_tcstoul(pszData, &pszEnd, 16);
		pszData = pszEnd;
		if (val > 0xFFFF) break;
		data[i] = (USHORT)val;
	}
	return i;
}

int HexStringToArray (LPCTSTR pszData, std::vector<UCHAR>& data)
{
	data.clear();

	while(1) {
		while (*pszData && !::_istxdigit(*pszData)) ++pszData;
		if (*pszData == 0) break;
		LPTSTR pszEnd = 0;
		ULONG val = ::_tcstoul(pszData, &pszEnd, 16);
		pszData = pszEnd;
		if (val > 0xFF) break;
		data.push_back((UCHAR)val);
	}
	return (int)data.size();
}

CString HexArrayToString (UCHAR data[], int length)
{
	CString s;
	LPTSTR buf = s.GetBuffer((3 * length + 1) * sizeof TCHAR);
	int nCharsWritten = 0;
	for (int i = 0; i < length; ++i) {
		nCharsWritten += ::_sntprintf_s(buf + nCharsWritten, 4, 3, _T("%02X "), data[i]);
	}
	buf[nCharsWritten] = 0;
	s.ReleaseBuffer();
	return s;
}

CString HexFillString (UCHAR cFill, int length)
{
	CString s;
	LPTSTR buf = s.GetBuffer((3 * length + 1) * sizeof TCHAR);
	int nCharsWritten = 0;
	for (int i = 0; i < length; ++i) {
		nCharsWritten += ::_sntprintf_s(buf + nCharsWritten, 4, 3, _T("%02X "), cFill);
	}
	buf[nCharsWritten] = 0;
	s.ReleaseBuffer();
	return s;
}
