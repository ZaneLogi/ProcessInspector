#pragma once

#define MONITOR_CENTER   0x0001        // center rect to monitor
#define MONITOR_CLIP     0x0000        // clip rect to monitor
#define MONITOR_WORKAREA 0x0002        // use monitor work area
#define MONITOR_AREA     0x0000        // use monitor entire area

void ClipOrCenterRectToMonitor(LPRECT prc, UINT flags);
void ClipOrCenterWindowToMonitor(HWND hwnd, UINT flags);