/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	Taskbar Properties
 * PURPOSE:	Application entry point
 *
 * PROGRAMMER:	Franco Tortoriello (torto09@gmail.com)
 */

 /* Enable ability to use visual styles */
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "tbconf.h"

void __stdcall main(void)
{
    hInstance = GetModuleHandleW(NULL);

    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_TASKBAR), NULL, TaskbarSettingsDlgProc, 0);
    ExitProcess(0);
}