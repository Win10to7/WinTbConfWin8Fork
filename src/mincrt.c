/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Replacement functions for building without linking to the VC CRT
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "mincrt.h"

#if defined(_MSC_VER)
#pragma function(memset)
#define min_memset memset
_Post_equal_to_(_Dst)
_At_buffer_(
    (unsigned char *)_Dst,
    _Iter_,
    _Size,
    _Post_satisfies_(((unsigned char *)_Dst)[_Iter_] == _Val)
)
void *min_memset(
#else  /* defined(__GNUC__) */
__attribute__((always_inline))
inline void *memset(
#endif
    _Out_writes_bytes_all_(_Size) void *_Dst,
    _In_                          int    _Val,
    _In_                          size_t _Size
)
{
#if defined(_MSC_VER)
    /* Ignore unitialized memory warning */
#pragma warning(push)
#pragma warning(disable: 6001)
#endif
    unsigned char *_Dst8 = (unsigned char *)_Dst;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

    while (_Size--)
        *_Dst8++ = (unsigned char)_Val;

    return _Dst;
}
