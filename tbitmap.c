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
 * tbitmap.c: 3 level mutibit trie based bitmap
 */

#include <assert.h>
#include <string.h>
#include "tbitmap.h"

/*
 * writePtrTag() produces the following warning:
 *   warning: dereferencing type-punned pointer will break
 *   strict-aliasing rules [-Wstrict-aliasing]
 * but this is intentional.
 * Suppress -Wunused-function (it is inline)
 */
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wunused-function"


#define ALLOC_MEM(_arg0_, _size_) malloc((_size_))
#define FREE_MEM(_arg0_, _ptr_)   free((_ptr_))
#define TBITMAP_ASSERT(_exp_)   assert((_exp_))

typedef struct strideLen_ {
    u8 sl0;
    u8 sl1;
    u8 sl2;
} strideLen;


static inline u8
maxNbits (void)
{
    return ((sizeof(((tBitMapL2*)0)->bitmap[0])) << 3);
}
static inline u32
posMask (void)
{
    return (u32)(maxNbits() - 1);
}
static inline u8
getPos (u32 bitPos)
{
    return (u8)(bitPos & posMask());
}
static inline u32
nL1elm (mtrie3l* p)
{
    return (1 << p->len[1]);
}
static inline u32
nL2elm (mtrie3l* p)
{
    return (1 << p->len[2]);
}
#if 0
static inline u32
setBits32 (u32 start, u32 end)
{
    return (end < 31) ? (((~0) << start) & (((1 << (end + 1)) - 1))) :
                        (((~0) << start) & (~0));
}
#endif
static inline u32
setBits32 (u32 start, u32 end)
{
    return (u32)(((~0) << start) & ((((u64)1) << (end + 1)) - 1));
}
static inline void
tBitMapFlip (tBitMap* p)
{
    p->flags |=  TBITMAP_IS_FLIPPED;
}
static inline void
tBitMapUnflip (tBitMap* p)
{
    p->flags &=  ~TBITMAP_IS_FLIPPED;
}
static inline bool
tBitMapIsFlipped (tBitMap* p)
{
    return (p->flags & TBITMAP_IS_FLIPPED) ? TRUE : FALSE;
}

/*
 * Array of stride length for trie.
 *
 * Ex: Use Strides[2] if 2^14 <= max bit position < 2^15
 *     and create a 3 level trie whose stride legths are
 *     level 0: 4 bits, level 1: 3 bits, level 2: 2 bits.
 */
static strideLen Strides[] = {
    {3, 2, 2},                 /*  0: index:  7 bits, bit-pos: 12 bits */
    {4, 2, 2},                 /*  1: index:  8 bits, bit-pos: 13 bits */
    {4, 3, 2},                 /*  2: index:  9 bits, bit-pos: 14 bits */
    {4, 3, 3},                 /*  3: index: 10 bits, bit-pos: 15 bits */
    {4, 4, 3},                 /*  4: index: 11 bits, bit-pos: 16 bits */
    {5, 4, 3},                 /*  5: index: 12 bits, bit-pos: 17 bits */
    {5, 4, 4},                 /*  6: index: 13 bits, bit-pos: 18 bits */
    {5, 5, 4},                 /*  7: index: 14 bits, bit-pos: 19 bits */
    {5, 5, 5},                 /*  8: index: 15 bits, bit-pos: 10 bits */
    {6, 5, 5},                 /*  9: index: 16 bits, bit-pos: 21 bits */
    {6, 6, 5},                 /* 10: index: 17 bits, bit-pos: 12 bits */
    {6, 6, 6},                 /* 11: index: 18 bits, bit-pos: 23 bits */
    {7, 6, 6},                 /* 12: index: 19 bits, bit-pos: 24 bits */
    {7, 7, 6},                 /* 13: index: 20 bits, bit-pos: 25 bits */
    {7, 7, 7},                 /* 14: index: 21 bits, bit-pos: 26 bits */
    {8, 7, 7},                 /* 15: index: 22 bits, bit-pos: 27 bits */
    {8, 8, 7},                 /* 16: index: 23 bits, bit-pos: 28 bits */
    {8, 8, 8},                 /* 17: index: 24 bits, bit-pos: 29 bits */
};
enum {
    TBITMAP_MIN_BITS = 12,      /* min bit length for a bitmap */
    TBITMAP_MAX_BITS = 29,      /* max bit length for a bitmap  */
};


/*
 * MSB of tBitMap: (sl0 + sl1 + sl2 + 5) - 1
 * The least significant 5 bits (0-31) are used to indicate
 * the bit position of a leaf bitmap in the level 2 node.
 *
 * pMap->pTrie->cnt is not used in this library.
 * Total number of L1 and L2 nodes: p->pTrie->nL1 + p->pTrie->nL2
 */
static tBitMap*
tBitMapAllocRaw (u8 sl0,  /* L0 stride length */
                 u8 sl1,  /* L1 stride length */
                 u8 sl2)  /* L2 stride length */
{
    tBitMap* pMap;
    
    pMap = ALLOC_MEM(MEM_TBITMAP, sizeof(*pMap));
    if (!pMap) {
        return NULL;
    }
    pMap->pTrie = mtrie3lAlloc(sl0, sl1, sl2);
    if (!pMap->pTrie) {
        return NULL;
    }
    pMap->flags  = 0;
    pMap->maxPos = (1 << (sl0 + sl1 + sl2 + 5)) - 1;

    return pMap;
}

tBitMap*
tBitMapAlloc (u32 maxBitPos)
{
    strideLen* p;
    u32 mask;
    int i;

    /*
     * Return error if maxBitPos is too large.
     */
    for (mask = 1 << 31; mask >= (1 << TBITMAP_MAX_BITS); mask >>= 1) {
        if (mask & maxBitPos) {
            return NULL; /* maxBitPos too large */
        }
    }
    /*
     * Find most significant set bit in maxBitPos.
     */
    for (i = elementsOf(Strides) - 1; i > 0; mask >>= 1) {
        if (mask & maxBitPos) {
            break;
        }
    }
    p = Strides + i;
    return tBitMapAllocRaw(p->sl0, p->sl1, p->sl2);
}

static int
tBitMapDestroy (tBitMap* pMap)
{
    u8    l0i, l0n;
    u8    l1i, l1n;
    u8    cnt[2]; /* # of remaining entries to process at level `i' */
    mtrie3l*    p;
    mtrie3l_l1* pl1;
    tBitMapL2*  pl2;

    if (!pMap) {
        return TBITMAP_ERR;
    }

    p   = pMap->pTrie;
    l0n = 1 << p->len[0];       /* # of level 0 entries */
    l1n = 1 << p->len[1];       /* # of level 1 entries */
    cnt[0] = p->nL1 + p->nL2;
    for (l0i = 0; l0i < l0n; ++l0i) {
        if (cnt[0] == 0) {
            return TBITMAP_SUCCESS; /* optimization */
        }
        if (!p->l0[l0i]) {
            continue;
        }
        pl1    = p->l0[l0i];
        cnt[1] = pl1->cnt;  /* # of L2 nodes incl. compressed nodes */
        for (l1i = 0; l1i < l1n; ++l1i) {
            if (cnt[1] == 0) {
                break;          /* optimization */
            }
            pl2 = getPtr(tBitMapL2, pl1->l1[l1i]); 
            if (!pl2) {
                continue;
            }
            cnt[1]--;
            FREE_MEM(MEM_TBITMAP, pl2);
        }
        cnt[0]--;
        FREE_MEM(MEM_TBITMAP, pl1);
    }
    return TBITMAP_SUCCESS;
}

int
tBitMapFree (tBitMap* pMap)
{
    int rt;

    if (!pMap) {
        return TBITMAP_ERR;
    }
    rt = tBitMapDestroy(pMap);
    FREE_MEM(MEM_TBITMAP, pMap);
    return rt;
}

/*
 * Reset (unset) bits between bit position `pos' and `endPos'
 * of the entry in the level 2 node.
 */
static int
tBitMapResetL2ent (tBitMap* pMap,
                   u16 l0i, u16 l1i, u16 l2i,
                   u8 pos, u8 endPos)
{
    u32 bits;
    u32 bitmap;
    int len;
    mtrie3l*    p;
    mtrie3l_l1* pl1;
    tBitMapL2*  pl2;


    if (!pMap) {
        return TBITMAP_ERR;
    }
    if (endPos >= maxNbits())  {
        return TBITMAP_EINDEX;
    }
    if (pos >= maxNbits())  {
        return TBITMAP_EINDEX;
    }

    p   = pMap->pTrie;
    pl1 = p->l0[l0i];    /* level 1 node pointer */
    if (!pl1) {
        return TBITMAP_SUCCESS; /* already unset */
    }
    pl2 = getPtr(tBitMapL2, pl1->l1[l1i]); /* level 2 node pointer */
    if (pl2) {
        bitmap = pl2->bitmap[l2i];
    } else {        
        if (getPtrTag(pl1->l1[l1i]) == 0) {
            return TBITMAP_SUCCESS; /* already unset */
        }
        len = (1 << p->len[2]) * sizeof(u32);
        pl2 = ALLOC_MEM(MEM_TBITMAP, len + sizeof(tBitMapL2));
        if (!pl2) {
            return TBITMAP_ENOMEM;
        }
        pl2->cnt     = nL2elm(p);
        pl2->nSetAll = nL2elm(p);
        memset(pl2->bitmap, ~0, len);
        pl1->l1[l1i] = (mtrie3l_l2*)pl2;
        ++p->nL2;
        bitmap = ~0;
    }

    /*
     * Reset bit positions from pos to endPos
     */
    bits = setBits32(pos, endPos);

    if ((bits & (~bitmap)) == bits) {
        return TBITMAP_SUCCESS; /* All bits are already reset */
    }
    if (bitmap == ~0) {
        --pl2->nSetAll;
    }
    bitmap &= ~bits;
    if (bitmap == 0) {
        --pl2->cnt;           /* # of bitmaps at least 1 bit is set */
        TBITMAP_ASSERT(p->num);
        --p->num;       /* total # of bitmaps at least 1 bit is set */
    }
    if (pl2->cnt == 0) {
        FREE_MEM(MEM_TBITMAP, pl2);
        pl1->l1[l1i] = NULL;
        --pl1->cnt;             /* # of L2 nodes incl. compressed nodes */
        TBITMAP_ASSERT(p->nL2 > 0);
        --p->nL2;               /* total # of L2 nodes */

        if (pl1->cnt == 0) {
            FREE_MEM(MEM_TBITMAP, pl1);
            p->l0[l0i] = NULL;
            --p->nL1;           /* total # of L1 nodes */
        }
    } else {
        pl2->bitmap[l2i] = bitmap;
    }
    return TBITMAP_SUCCESS;
}

/*
 * Set bits between bit position `pos' and `endPos'
 * of the entry in the level 2 node.
 */
static int
tBitMapSetL2ent (tBitMap* pMap,
                 u16 l0i, u16 l1i, u16 l2i,
                 u8 pos, u8 endPos)
{
    u32 bits;
    u32 bitmap;
    int do_free = 0;
    int len;
    mtrie3l*    p;
    mtrie3l_l1* pl1;
    tBitMapL2*  pl2;

    if (!pMap) {
        return TBITMAP_ERR;
    }
    if (endPos >= maxNbits())  {
        return TBITMAP_EINDEX;
    }
    if (pos >= maxNbits())  {
        return TBITMAP_EINDEX;
    }

    p   = pMap->pTrie;
    pl1 = p->l0[l0i];        /* level 1 node pointer */
    if (pl1) {
        if (getPtrTag(pl1->l1[l1i])) {
            return TBITMAP_SUCCESS;            /* already set */
        }
        pl2 = getPtr(tBitMapL2, pl1->l1[l1i]); /* level 2 node pointer */
    } else {
        len = sizeof(mtrie3l_l1) + ((1 << p->len[1]) * sizeof(tBitMapL2*));
        pl1 = ALLOC_MEM(MEM_TBITMAP, len);
        if (!pl1) {
            return TBITMAP_ENOMEM;
        }
        memset(pl1, 0, len);
        p->l0[l0i] = pl1;
        ++p->nL1;               /* total number of L1 nodes */
        do_free = 1;
        pl2 = NULL;
    }

    bits = setBits32(pos, endPos);
    if (pl2) {
        bitmap = pl2->bitmap[l2i];
        if (bitmap & bits) {
            return TBITMAP_SUCCESS; /* already set */
        }
        if (bitmap == 0) {
            ++pl2->cnt; /* at least one bit will be set in bitmap[l2i] */
            ++p->num;   /* total # of bitmaps at least 1 bit is set */
        }
        bitmap |= bits;
        if (bitmap == ~0) {
            ++pl2->nSetAll;     /* # of bitmaps all bits are set */
        }
        /*
         * Compression:
         *  free the L2 node and mark the assocated L1 node entry
         *  if all L2 nodes' bitmap[]s are ~0.
         */
        if (pl2->nSetAll == nL2elm(p)) {
            FREE_MEM(MEM_TBITMAP, pl2);
            writePtrTag(&pl1->l1[l1i], 1);
            TBITMAP_ASSERT(p->nL2 > 0);
            --p->nL2;           /* total # of L2 nodes */
        } else {
            pl2->bitmap[l2i] = bitmap;
        }
    } else {
        len = sizeof(tBitMapL2) + ((1 << p->len[2]) * sizeof(u32));
        pl2 = ALLOC_MEM(MEM_TBITMAP, len);
        if (!pl2) {
            if (do_free) {
                FREE_MEM(MEM_TBITMAP, pl1);
                p->l0[l0i] = NULL;
                --p->nL1;       /* total # of L1 nodes */
            }
            return TBITMAP_ENOMEM;
        }
        memset(pl2, 0, len);
        pl1->l1[l1i] = (mtrie3l_l2*)pl2;
        ++pl1->cnt;             /* # of L2 nodes incl. compressed nodes */
        ++p->nL2;               /* total # of L2 nodes */
        pl2->bitmap[l2i] = bits;
        if (bits == ~0) {
            ++pl2->nSetAll;     /* # of bitmaps all bits are set */
        }
        ++pl2->cnt;     /* at least one bit is set in bitmap[l2i] */
        ++p->num;       /* total # of bitmaps at least 1 bit is set */
    }

    return TBITMAP_SUCCESS;
}

bool
tBitMapIsSet (tBitMap* pMap, u32 bitPos)
{
    u16 l0i;
    u16 l1i;
    u16 l2i;
    u32 index;
    u32 pos;
    mtrie3l*    p;
    mtrie3l_l1* pl1;
    tBitMapL2*  pl2;

    if (!pMap) {
        return TBITMAP_ERR;
    }
    if (bitPos > pMap->maxPos) {
        return TBITMAP_EINDEX;
    }

    index = bitPos >> 5;
    p = pMap->pTrie;
    MTRIE3L_GET_INDICES;

    pl1 = p->l0[l0i];    /* level 1 node pointer */
    if (!pl1) {
        return FALSE;
    }
    if (getPtrTag(pl1->l1[l1i])) {
        return TRUE;        /* all bits in pl1->l2[l1i] are set */
    }
    pl2 = getPtr(tBitMapL2, pl1->l1[l1i]); /* level 2 node pointer */
    if (!pl2) {
        return FALSE;
    }
    pos = bitPos & (~(((u32)(~0)) << 5));
    pos = 1 << pos;
    if (pl2->bitmap[l2i] & pos) {
        return TRUE;
    }
    return FALSE;
}

int
tBitMapSetResetBlock (tBitMap* pMap, u32 start, u32 end, bool isSet)
{
    u16 l0i, l0j;
    u16 l1i, l1j, l1n, l1nMax;
    u16 l2i, l2j, l2n, l2nMax;
    u32 index;
    u8  pos;
    u8  endPos;
    int len;
    int rt;
    mtrie3l*    p;
    mtrie3l_l1* pl1;
    tBitMapL2*  pl2;
    static int (*f)(tBitMap*, u16, u16, u16, u8, u8);

    if (!pMap) {
        return TBITMAP_ERR;
    }
    if (end > pMap->maxPos) {
        return TBITMAP_EINDEX;
    }
    if (start > end) {
        return TBITMAP_EINDEX;
    }
    f = (isSet) ? tBitMapSetL2ent : tBitMapResetL2ent;

    p = pMap->pTrie;
    index = end >> 5;
    MTRIE3L_GET_INDICES;
    l0j = l0i;
    l1j = l1i;
    l2j = l2i;
    index = start >> 5;
    MTRIE3L_GET_INDICES;

    assert (l0i <= l0j);

    pos    = getPos(start);
    endPos = getPos(end);
    l2nMax = nL2elm(p) - 1;
    if ((l0i == l0j) && (l1i == l1j)) {
        if (l2i == l2j) {
            /*
             * Call tBitMapSetL2ent() or tBitMapResetL2ent() and return
             * if only one L2 node needs to be processed.
             */
            return (*f)(pMap, l0i, l1i, l2i, pos, endPos);
        }
        l2n = l2j;
    } else {
        l2n = l2nMax;
    }
    /*
     * Process first index (l2i) in the 1st L2 node
     */
    rt = (*f)(pMap, l0i, l1i, l2i, pos, maxNbits() - 1);
    if (rt != TBITMAP_SUCCESS) {
        return rt;
    }
    /*
     * Process the indices from l2i+1 to l2n-1 in the 1st L2 node
     */
    for (++l2i; l2i < l2n; ++l2i) {
        rt = (*f)(pMap, l0i, l1i, l2i, 0, maxNbits() - 1);
        if (rt != TBITMAP_SUCCESS) {
            return rt;
        }
    }
    /*
     * Process the last indice (l2n) in the 1st L2 node
     */
    if ((l0i == l0j) && (l1i == l1j)) {
        /*
         * Last node to process. Set end position to endPos.
         */
        return (*f)(pMap, l0i, l1i, l2i, 0, endPos);
    } else {
        /*
         * If l2i > l2n, initial value of l2i was l2n.
         * In this case, index l2i is already processed above
         * so that we can skip this.
         */
        if (l2i == l2n) {
            rt = (*f)(pMap, l0i, l1i, l2i, 0, maxNbits() - 1);
            if (rt != TBITMAP_SUCCESS) {
                return rt;
            }
        }
    }
    /*
     * Process: from: L0[l0i], L1[l1i],   L2[0]
     *          to:   L0[l0j], L1[l1j-1], L2[0:l2nMax]
     */   
    l1nMax = nL1elm(p) - 1;
    if (l1i == l1nMax) {
        l1i = 0;
        ++l0i;
    } else {
        ++l1i;
    }
    len = sizeof(mtrie3l_l1) + ((1 << p->len[1]) * sizeof(tBitMapL2*));
    for (; l0i <= l0j; ++l0i) {
        if (p->num == 0) {
            return TBITMAP_SUCCESS;
        }
        pl1 = p->l0[l0i];
        if (pl1 == NULL) {
            if (isSet) {
                pl1 = ALLOC_MEM(MEM_TBITMAP, len);
                if (!pl1) {
                    return TBITMAP_ENOMEM;
                }
                memset(pl1, 0, len);
                p->l0[l0i] = pl1;
                ++p->nL1;
            } else {
                continue;
            }
        }
        if (l0i == l0j) {
            if (l1j == 0) {
                break;
            }
            l1n = l1j - 1;
        } else {
            l1n = l1nMax;
        }
        for (; l1i <= l1n; ++l1i) {
            pl2 = getPtr(tBitMapL2, pl1->l1[l1i]);
            if (pl2) {
                if (isSet) {
                    /*
                     * Mark `*pl2' is full.
                     */
                    writePtrTag(&pl1->l1[l1i], 1);
                    p->num += (nL2elm(p) - pl2->cnt);
                } else {
                    pl1->l1[l1i]  = NULL;
                    p->num       -= pl2->cnt;
                    --pl1->cnt; /* # of L2 nodes incl. compressed nodes */
                }
                FREE_MEM(MEM_TBITMAP, pl2);
                TBITMAP_ASSERT(p->nL2 > 0);
                --p->nL2;
            } else {
                if (isSet) {
                    writePtrTag(&pl1->l1[l1i], 1);
                    p->num += nL2elm(p);
                    ++pl1->cnt; /* # of L2 nodes incl. compressed nodes */
                } else {
                    if (getPtrTag(pl1->l1[l1i]) == 1) {
                        p->num -= nL2elm(p);
                        pl1->l1[l1i] = NULL;
                        --pl1->cnt; /* # of L2 nodes incl. compressed nodes */
                    }
                }
            }
            if ((!isSet) && (pl1->cnt == 0)) {
                FREE_MEM(MEM_TBITMAP, pl1);
                p->l0[l0i] = NULL;
                --p->nL1;
                break;   /* no more L1 nodes. Process next L0 index */
            }
        }
        if (l0i == l0j) {
            /*
             * Reached the last L1 node, i.e. L0[l0j], L1[l1j].
             * Break and process the remaining L2 nodes.
             */
            break;
        }
        l1i = 0;
    }
    /*
     * Process the indices from 0 to l2j-1 in the last L2 node
     */
    for (l2i = 0; l2i < l2j; ++l2i) {
        rt = (*f)(pMap, l0i, l1i, l2i, 0, maxNbits() - 1);
        if (rt != TBITMAP_SUCCESS) {
            return rt;
        }
    }
    return (*f)(pMap, l0i, l1i, l2i, 0, endPos);
}

int
tBitMapSetReset (tBitMap* pMap, u32 bitPos, bool isSet)
{
    u16 l0i;
    u16 l1i;
    u16 l2i;
    u32 index;
    u8  pos;
    mtrie3l*    p;

    if (!pMap) {
        return TBITMAP_ERR;
    }
    if (bitPos > pMap->maxPos) {
        return TBITMAP_EINDEX;
    }
    p = pMap->pTrie;
    index = bitPos >> 5;
    MTRIE3L_GET_INDICES;
    pos = getPos(bitPos);

    if (isSet) {
        return tBitMapSetL2ent (pMap, l0i, l1i, l2i, pos, pos);
    }
    return tBitMapResetL2ent (pMap, l0i, l1i, l2i, pos, pos);
}

int
tBitMapSetResetAll (tBitMap* pMap, bool isSet)
{
    int rt;

    rt = tBitMapDestroy(pMap);
    if (rt != TBITMAP_SUCCESS) {
        return rt;
    }
    if (isSet) {
        tBitMapFlip(pMap);
    } else {
        tBitMapUnflip(pMap);
    }
    return TBITMAP_SUCCESS;
}
