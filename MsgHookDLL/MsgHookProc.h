#ifdef MSGHOOKDLL_EXPORTS
#define MSGHOOKDLL_API __declspec(dllexport)
#else
#define MSGHOOKDLL_API __declspec(dllimport)
#endif

extern "C" MSGHOOKDLL_API LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam);
