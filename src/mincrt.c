/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	Taskbar Properties
 * PURPOSE:	Replacement function for building without the Visual C CRT
 */

#include <ctype.h>

#pragma function(memset)
void* memset(void* dst, int c, size_t count)
{
    unsigned char* dst8 = dst;

    while (count--)
        *dst8++ = (unsigned char)c;

    return dst;
}
