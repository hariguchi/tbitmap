#ifndef __TBITMAP_H__
#define __TBITMAP_H__

/*
 * Copyright (c) 2017 Yoichi Hariguchi
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall
 * be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * tbitmap.h: 3 level mutibit trie based bitmap
 */

#include "mtrie3l.h"

/*
 * Trie bitmap definition
 */
typedef struct tBitMap_ {
    u32      flags;             /* See the enum below */
    u32      maxPos;            /* max bit position */
    mtrie3l* pTrie;
} tBitMap;
enum {
    TBITMAP_IS_FLIPPED = 1, /* bit 0: set if bitmap is inverted (flipped) */
};

/*
 * Level 2 node
 */
typedef struct tBitMapL2_ {
    u16 cnt;       /* number of bitmaps wherein at least one bit is set */
    u16 nSetAll;   /* number of bitmaps wherein all bits are set */
    u32 bitmap[0]; /* bitmaps */
} tBitMapL2;

enum {
    TBITMAP_SUCCESS   =  0,
    TBITMAP_ERR       = -1,     /* generic error */
    TBITMAP_ENOMEM    = -2,     /* no memory */
    TBITMAP_EINDEX    = -3,     /* index too big */
    TBITMAP_EOCCUPIED = -4,     /* leaf already exists */
    TBITMAP_ETABLE    = -5,     /* table not empty */
    TBITMAP_ESLEN     = -6,     /* too large stride length */
    TBITMAP_EBITPOS   = -7,     /* too large bit position */
};


/*
 * Function prototypes
 */
tBitMap* tBitMapAlloc (u32 maxitPos);
int      tBitMapFree (tBitMap* pMap);
int      tBitMapSetResetBlock (tBitMap* pMap, u32 start, u32 end, bool isSet);
int      tBitMapSetReset (tBitMap* pMap, u32 bitPos, bool isSet);
int      tBitMapSetResetAll (tBitMap* pMap, bool isSet);
bool     tBitMapIsSet (tBitMap* pMap, u32 bitPos);
const revNum* tBitMapRevision (void);
const char*   tBitMapCompilationDate (void);


/*
 * Inline functions
 */
static inline int
tBitMapSet (tBitMap* pMap, u32 bitPos)
{
    return tBitMapSetReset(pMap, bitPos, TRUE);
}
static inline int
tBitMapReset (tBitMap* pMap, u32 bitPos)
{
    return tBitMapSetReset(pMap, bitPos, FALSE);
}
static inline int
tBitMapSetBlock (tBitMap* pMap, u32 start, u32 end)
{
    return tBitMapSetResetBlock(pMap, start, end, TRUE);
}
static inline int
tBitMapResetBlock (tBitMap* pMap, u32 start, u32 end)
{
    return tBitMapSetResetBlock(pMap, start, end, FALSE);
}
static inline int
tBitMapSetAll (tBitMap* pMap)
{
    return tBitMapSetResetAll(pMap, TRUE);
}
static inline int
tBitMapResetAll (tBitMap* pMap)
{
    return tBitMapSetResetAll(pMap, FALSE);
}

#endif /* __TBITMAP_H__ */
