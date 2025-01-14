#pragma once
#if !defined(MINCRT_H)
#define MINCRT_H

#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if !defined(_MSC_VER)

void *min_memset(
    void *_Dst,
    int _Val,
    size_t _Size
);

#endif  /* !defined(_MSC_VER) */

#endif  /* !defined(MINCRT_H) */
