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
*/

#include <assert.h>
#include <string.h>
#include "mtrie3l.h"

enum {
    MTRIE3L_KEEP_ENT = 0,
    MTRIE3L_DEL_ENT  = 1,
    MTRIE3L_MAX_STRIDE_LEN = sizeof(((mtrie3l*)0)->cnt) << 3,
};

#define ALLOC_MEM(_arg0, _size) (malloc ((_size)))
#define FREE_MEM(_arg0, _ptr)   (free ((_ptr)))
#define MTRIE3L_ASSERT(_exp_)   assert((_exp_))


mtrie3l*
mtrie3lAlloc (u8 sl0,           /* level 0 stride length */
              u8 sl1,           /* level 1 stride length */
              u8 sl2)           /* level 2 stride length */
{
    mtrie3l* p = NULL;          /* trie head */
    int      len;

    /*
     * Max number of level 0 nodes (p->cnt) is used
     * to calculate MTRIE3L_MAX_STRIDE_LEN. So it must be begreater than
     * or equal to max mumber of both level 1 and level 2 nodes.
     */
    MTRIE3L_ASSERT(sizeof(p->cnt) >= sizeof(((mtrie3l_l1*)0)->cnt));
    MTRIE3L_ASSERT(sizeof(p->cnt) >= sizeof(((mtrie3l_l2*)0)->cnt));

    if (sl0 > MTRIE3L_MAX_STRIDE_LEN) {
        return NULL;
    }
    if (sl1 > MTRIE3L_MAX_STRIDE_LEN) {
        return NULL;
    }
    if (sl2 > MTRIE3L_MAX_STRIDE_LEN) {
        return NULL;
    }
    len = sizeof(mtrie3l) + ((1 << sl0) * sizeof(mtrie3l_l1*));
    p = ALLOC_MEM(0, len);
    if (!p) {
        return p;
    }
    memset(p, 0, len);

    p->len[0] = sl0;
    p->len[1] = sl1;
    p->len[2] = sl2;
    p->slen   = sl0 + sl1 + sl2;
    return p;
}

int
mtrie3lFree (mtrie3l* p)
{
    if (!p) {
        return MTRIE3L_ERR;
    }
    if (p->num) {
        return MTRIE3L_ETABLE;
    }
    FREE_MEM(0, p);
    return MTRIE3L_SUCCESS;
}

int
mtrie3lInsert (mtrie3l* p, u32 index, void* pEnt)
{
    u16         l0i;
    u16         l1i;
    u16         l2i;
    int         len;
    int         do_free = 0;
    mtrie3l_l1* pl1;
    mtrie3l_l2* pl2;

    if (!p) {
        return MTRIE3L_ERR;
    }
    if (index > mtrie3lGetMaxIndex(p)) {
        return MTRIE3L_EINDEX;
    }

    MTRIE3L_GET_INDICES;

    pl1 = p->l0[l0i];           /* level 1 node pointer */
    if (!pl1) {
        len = sizeof(mtrie3l_l1) + ((1 << p->len[1]) * sizeof(mtrie3l_l2*));
        pl1 = ALLOC_MEM(0, len);
        if (!pl1) {
            return MTRIE3L_ENOMEM;
        }
        memset(pl1, 0, len);
        p->l0[l0i] = pl1;
        ++p->cnt;
        ++p->nL1;
        do_free = 1;
    }
    pl2 = pl1->l1[l1i];         /* level 2 node pointer */
    if (pl2) {
        if (pl2->l2[l2i]) {
            return MTRIE3L_EOCCUPIED; /* already occupied */
        }
    } else {
        len = sizeof(mtrie3l_l2) + ((1 << p->len[2]) * sizeof(void*));
        pl2 = ALLOC_MEM(0, len);
        if (!pl2) {
            if (do_free) {
                FREE_MEM(0, pl1);
                p->l0[l0i] = NULL;
                --p->cnt;
                --p->nL1;
            }
            return MTRIE3L_ENOMEM;
        }
        memset(pl2, 0, len);
        pl1->l1[l1i] = pl2;
        ++pl1->cnt;
        ++p->nL2;
    }
    pl2->l2[l2i] = pEnt;
    ++pl2->cnt;
    ++p->num;
    return MTRIE3L_SUCCESS;
}

void*
mtrie3lDelete (mtrie3l* p, u32 index)
{
    u16         l0i;
    u16         l1i;
    u16         l2i;
    mtrie3l_l1* pl1;
    mtrie3l_l2* pl2;
    void*       pEnt;

    if (!p) {
        return NULL;
    }
    if (index > mtrie3lGetMaxIndex(p)) {
        return NULL;
    }

    MTRIE3L_GET_INDICES;

    pl1 = p->l0[l0i];
    if (!pl1) {
        return NULL;
    }
    pl2 = pl1->l1[l1i];
    if (!pl2) {
        return NULL;
    }
    pEnt = pl2->l2[l2i];
    if (!pEnt) {
        return NULL;
    }
    /*
     * Found an enty to delete.
     *  1. Decrement total # of entries
     *  2. Decrement # of entries in the level 2 node
     *  3. If no entries in the level 2 node, free it, then
     *  4.  Decrement # of entries in the level 1 node
     *  5.  If no entries in the level 1 node, free it, then
     *  6.   Decrement # of entries in the level 0 node
     *  7. Return the entry pointer
     */
    --p->num;
    pl2->l2[l2i] = NULL;
    --pl2->cnt;
    if (pl2->cnt == 0) {
        FREE_MEM(0, pl2);
        pl1->l1[l1i] = NULL;
        --pl1->cnt;
        --p->nL2;

        if (pl1->cnt == 0) {
            FREE_MEM(0, pl1);
            p->l0[l0i] = NULL;
            --p->cnt;
            --p->nL1;
        }
    }
    return pEnt;
}

void*
mtrie3lFind (mtrie3l* p, u32 index)
{
    u16 l0i;
    u16 l1i;
    u16 l2i;

    if (!p) {
        return NULL;
    }
    if (index > mtrie3lGetMaxIndex(p)) {
        return NULL;
    }

    MTRIE3L_GET_INDICES;

    if (!p->l0[l0i]) {
        return NULL;
    }
    if (!(p->l0[l0i])->l1[l1i]) {
        return NULL;
    }
    return ((p->l0[l0i])->l1[l1i])->l2[l2i];
}

void*
mtrie3lFindNext (mtrie3l* p, u32* pIndex)
{
    u16         l0n, l1n, l2n;
    u16         l0i, l1i, l2i;
    u32         idx, index;
    u8          cnt[2]; /* # of remaining entries to process at level `i' */
    mtrie3l_l1* pl1;
    mtrie3l_l2* pl2;

    if (!p) {
        goto out;
    }
    MTRIE3L_ASSERT(*pIndex <= mtrie3lGetMaxIndex(p));

    l0n   = 1 << p->len[0];     /* # of level 0 entries */
    l1n   = 1 << p->len[1];     /* # of level 1 entries */
    l2n   = 1 << p->len[2];     /* # of level 2 entries */
    index = *pIndex;

    MTRIE3L_GET_INDICES;

    cnt[0] = p->cnt;
    for (; l0i < l0n; ++l0i) {
        if (cnt[0] == 0) {
            return NULL;
        }
        if (!p->l0[l0i]) {
            continue;
        }
        idx = l0i << (p->len[1] + p->len[2]);
        pl1 = p->l0[l0i];
        cnt[0]--;
        cnt[1] = pl1->cnt;
        for (; l1i < l1n; ++l1i) {
            if (cnt[1] == 0) {
                break;
            }
            if (!pl1->l1[l1i]) {
                continue;
            }
            idx &= ~((1 << (p->len[1] + p->len[2])) - 1);
            idx |= (l1i << p->len[2]);
            idx |= l2i;
            pl2 = pl1->l1[l1i];
            cnt[1]--;
            for (; l2i < l2n; ++l2i) {
                if (pl2->l2[l2i]) {
                    *pIndex = idx;
                    return pl2->l2[l2i];
                }
                ++idx;
            }
            l2i = 0;
        }
        l1i = 0;
    }
out:
    *pIndex = (1 << p->slen);
    return NULL;
}

void*
mtrie3lFindPrev (mtrie3l* p, u32* pIndex)
{
    u16         l1n, l2n;
    s32         l0i, l1i, l2i;
    u32         idx, index;
    u8          cnt[2]; /* # of remaining entries to process at level `i' */
    mtrie3l_l1* pl1;
    mtrie3l_l2* pl2;

    MTRIE3L_ASSERT(*pIndex <= mtrie3lGetMaxIndex(p));

    if (!p) {
        goto out;
    }

    l1n = (1 << p->len[1]) - 1; /* max level 1 index */
    l2n = (1 << p->len[2]) - 1; /* max level 2 index */
    index = *pIndex;

    MTRIE3L_GET_INDICES;

    cnt[0] = p->cnt;
    for (; l0i >= 0; --l0i) {
        if (cnt[0] == 0) {
            return NULL;
        }
        if (!p->l0[l0i]) {
            continue;
        }
        idx = l0i << (p->len[1] + p->len[2]);
        pl1 = p->l0[l0i];
        cnt[0]--;
        cnt[1] = pl1->cnt;
        for (; l1i >= 0; --l1i) {
            if (cnt[1] == 0) {
                break;
            }
            if (!pl1->l1[l1i]) {
                continue;
            }
            idx &= ~((1 << (p->len[1] + p->len[2])) - 1);
            idx |= (l1i << p->len[2]);
            idx |= l2i;
            pl2 = pl1->l1[l1i];
            cnt[1]--;
            for (; l2i >= 0; --l2i) {
                if (pl2->l2[l2i]) {
                    *pIndex = idx;
                    return pl2->l2[l2i];
                }
                --idx;
            }
            l2i = l2n;
        }
        l1i = l1n;
    }
out:
    *pIndex = 0;
    return NULL;
}

static int
mtrie3lWalkInternal (mtrie3l* p, void *pData,
                     void (*f)(u32, void*, void*), int free_node)
{
    u8    l0i, l0n;
    u8    l1i, l1n;
    u8    l2i, l2n;
    u8    cnt[3]; /* # of remaining entries to process at level `i' */
    u32   idx;
    mtrie3l_l1* pl1;
    mtrie3l_l2* pl2;

    if ((!p) || (!f)) {
        return MTRIE3L_ERR;
    }

    l0n = 1 << p->len[0];       /* # of level 0 entries */
    l1n = 1 << p->len[1];       /* # of level 1 entries */
    l2n = 1 << p->len[2];       /* # of level 2 entries */
    cnt[0] = p->cnt;
    for (l0i = 0; l0i < l0n; ++l0i) {
        if (cnt[0] == 0) {
            if (free_node) {
                /*
                 * reset level 0 counters
                 */
                p->cnt = 0;
                p->num = 0;
            }
            return MTRIE3L_SUCCESS; /* optimization */
        }
        if (!p->l0[l0i]) {
            continue;
        }
        idx = l0i << (p->len[1] + p->len[2]);
        pl1 = p->l0[l0i];
        cnt[0]--;
        cnt[1] = pl1->cnt;
        for (l1i = 0; l1i < l1n; ++l1i) {
            if (cnt[1] == 0) {
                break;
            }
            if (!pl1->l1[l1i]) {
                continue;
            }
            idx &= ~((1 << (p->len[1] + p->len[2])) - 1);
            idx |= (l1i << p->len[2]);
            pl2 = pl1->l1[l1i];
            cnt[1]--;
            cnt[2] = pl2->cnt;
            for (l2i = 0; l2i < l2n; ++l2i) {
                if (cnt[2] == 0) {
                    break;
                }
                if (pl2->l2[l2i]) {
                    (*f)(idx, pData, pl2->l2[l2i]);
                    --cnt[2];
                }
                ++idx;
            }
            if (free_node) {
                /*
                 * no need to set pl1->l1[l1i] to NULL
                 * since the node is freed anyway.
                 */
                FREE_MEM(0, pl2);
            }
        }
        if (free_node) {
            /*
             * Need to set p->l0[l0i] to NULL because
             * the trie itself may not be freed.
             */
            FREE_MEM(0, pl1);
            p->l0[l0i] = NULL;
        }
    }
    return MTRIE3L_SUCCESS;
}

int
mtrie3lWalk (mtrie3l* p, void *pData,
             void (*f)(u32 index, void* pData, void* pEnt))
{
    return mtrie3lWalkInternal(p, pData, f, MTRIE3L_KEEP_ENT);
}

int
mtrie3lDeleteAll (mtrie3l* p,
                  void (*delEnt)(u32 index, void* dummy, void* pEnt))
{
    return mtrie3lWalkInternal(p, NULL, delEnt, MTRIE3L_DEL_ENT);
}
