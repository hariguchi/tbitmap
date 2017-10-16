#ifndef __MTRIE3L_H__
#define __MTRIE3L_H__

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
 * mtrie3l.h: 3 level mutibit trie
 */

#include "local_types.h"


typedef struct mtrie3l_l0_ mtrie3l;


/*
 * Level 2 node
 */
typedef struct mtrie3l_l2_ {
    u16   cnt;                  /* number of leaves in this node */
    void* l2[0];                /* array of pointers to leaves */
} mtrie3l_l2;

/*
 * Level 1 node
 */
typedef struct trie3l_l1_ {
    u16         cnt;            /* number of subnodes in this node */
    mtrie3l_l2* l1[0];          /* array of pointers to L2 nodes */
} mtrie3l_l1;

/*
 * 3 level multibit trie: Level 0 node
 */
struct mtrie3l_l0_ {
    u32         num;         /* total number of leaves in this trie */
    u16         cnt;         /* number of subnodes in this node */
    u16         slen;        /* total stride length */
    u32         nL1;         /* number of level 1 nodes */
    u32         nL2;         /* number of level 2 nodes */
    void*       pPriv;       /* pointer for user's private data */
    u8          len[3];      /* stride length for L0, L1, and L2 */
    mtrie3l_l1* l0[0];       /* array of pointers to L1 nodes */
};

enum {
    MTRIE3L_SUCCESS   =  0,
    MTRIE3L_ERR       = -1,     /* generic error */
    MTRIE3L_ENOMEM    = -2,     /* no memory */
    MTRIE3L_EINDEX    = -3,     /* index too big */
    MTRIE3L_EOCCUPIED = -4,     /* leaf already exists */
    MTRIE3L_ETABLE    = -5,     /* table not empty */
    MTRIE3L_ESLEN     = -6,     /* too large stride length */
};

#define MTRIE3L_GET_INDICES \
do {\
    l0i = (index >> (p->len[1]+p->len[2])) & ((1 << p->len[0]) - 1);\
    l1i = (index >> p->len[2]) & ((1 << p->len[1]) - 1);\
    l2i = index & ((1 << p->len[2]) - 1);\
} while (0)

#ifndef MTRIE3L_READ_LOCK
#define MTRIE3L_READ_LOCK(_p_)   /* lock the trie from reading */
#endif
#ifndef MTRIE3L_READ_UNLOCK
#define MTRIE3L_READ_UNLOCK(_p_) /* unlock the trie from reading */
#endif

/*
 * Function prototypes
 */
mtrie3l* mtrie3lAlloc  (u8 sl0,  /* L0 stride length */
                        u8 sl1,  /* L1 stride length */
                        u8 sl2); /* L2 stride length */
int   mtrie3lFree      (mtrie3l *p);
int   mtrie3lInsert    (mtrie3l* p, u32 index, void* pleaf);
void* mtrie3lDelete    (mtrie3l* p, u32 index);
int   mtrie3lDeleteAll (mtrie3l* p,
                        void (*delEnt)(u32 index, void *dummy, void* pEnt));
int   mtrie3lWalk      (mtrie3l* p, void *pData,
                        void (*f)(u32 index, void* pData, void* pEnt));
void* mtrie3lFind      (mtrie3l* p, u32 index);
void* mtrie3lFindNext  (mtrie3l* p, u32* pIndex);
void* mtrie3lFindPrev  (mtrie3l* p, u32* pIndex);

const revNum* mtrie3lRevision        (void);
const char*   mtrie3lCompilationDate (void);



static inline u32
mtrie3lGetMaxIndex (mtrie3l* p)
{
    return ((1 << p->slen) - 1);
}

static inline u32
mtrie3lNumEntries (mtrie3l* p)
{
    return p->num;
}

static inline u32
mtrie3lNumL1 (mtrie3l* p)
{
    return p->nL1;
}

static inline u32
mtrie3lNumL2 (mtrie3l* p)
{
    return p->nL2;
}

static inline u32
mtrie3lL0nodeSize (mtrie3l* p)
{
    return sizeof(mtrie3l) + ((1 << p->len[0])*sizeof(mtrie3l_l1*));
}

static inline u32
mtrie3lL1nodeSize (mtrie3l* p)
{
    return sizeof(mtrie3l_l1) + ((1 << p->len[1])*sizeof(mtrie3l_l2*));
}

static inline u32
mtrie3lL2nodeSize (mtrie3l* p)
{
    return sizeof(mtrie3l_l2) + ((1 << p->len[2])*sizeof(void*));
}

static inline u32
mtrie3lNbytesL0 (mtrie3l* p)
{
    return mtrie3lL0nodeSize(p);
}

static inline u32
mtrie3lNbytesL1 (mtrie3l* p)
{
    return mtrie3lNumL1(p) * mtrie3lL1nodeSize(p);
}

static inline u32
mtrie3lNbytesL2 (mtrie3l* p)
{
    return mtrie3lNumL2(p) * mtrie3lL2nodeSize(p);
}


/*
 * mtrie3l 
 *
 * 3 level multibit trie
 *
 * 1. Creation
 *
 * mtrie3l* mtrie3Alloc(u8 sl0, u8 sl1, u8 sl2);
 *
 *  input:
 *    sl0: level 0 stride length (index).
 *    sl1: level 1 stride length (index).
 *    sl1: level 2 stride length (index).
 *
 *  return:
 *    NULL if there is no memory.
 *    Valid pointer of the trie otherwise.
 *
 *  output:
 *    None.
 *
 *
 * 2. Table Destruction
 *
 * mtrie3l* mtrie3Free (mtrie3l *pTbl)
 *
 *  input:
 *    pTbl: pointer to the trie to be destroyed
 *
 *  return:
 *    MTRIE3L_SUCCESS: successfully destroyed.
 *    MTRIE3L_ETABLE:  trie is not empty. Call mtrie3lDeleteAll() first.
 *    MTRIE3L_ERR:     pTbl is NULL.
 *
 *  output:
 *    None.
 *
 *
 * 3. Entry Insertion
 *
 * int mtrie3lInsert (mtrie3l* pTbl, u32 index, void* pEnt);
 *
 *  input:
 *    pTbl:  pointer to a multibit trie.
 *    index: index to insert an entry pointer (pEnt).
 *    pEnt:  pointer to an entry to be inserted.
 *
 *  return:
 *    MTRIE3L_SUCCESS:   successfully inserted.
 *    MTRIE3L_ERR:       pTbl is NULL.
 *    MTRIE3L_EINDEX:    index is too large.
 *    MTRIE3L_ENOMEM:    no memory to allocate a trie node.
 *    MTRIE3L_EOCCUPIED: index is already occupied
 *
 *  output:
 *    None.
 *
 *
 * 4. Entry Deletion
 *
 * void* mtrie3lDelete (mtrie3l* p, u32 index)
 *
 *  input:
 *    pTbl:  pointer to a multibit trie.
 *    index: index to remove an entry pointer from the trie.
 *
 *  return:
 *    NULL if there is no entry at the index.
 *    Valid pointer otherwise.
 *
 *  output:
 *    None.
 *
 *
 * 5. Entry Lookup
 *
 * void* mtrie3lFind (mtrie3l* p, u32 index)
 *
 *  input:
 *    pTbl:  pointer to a multibit trie.
 *    index: index to find an entry pointer
 *
 *  return:
 *    NULL if there is no entry at the index.
 *    Valid pointer otherwise.
 *
 *  output:
 *    None.
 *
 *
 * 6. Trie Walk-through
 *
 * int mtrie3lWalk (mtrie3l* pTbl, void* pData,
 *                  void (*f)(u32 index, void* pData, void* pEnt))
 *
 *  input:
 *    pTbl: pointer to a multibit trie.
 *    f:    function pointer called when index 'index' has an entry pointer.
 *          (*f)() has the following parameters:
 *            - index: index at which pEnt is located.
 *            - pData: same value as the 2nd parameter to mtrie3Walk().
 *            - pEnt:  pointer at index 'index'.
 *
 *  return:
 *    MTRIE3L_SUCCESS: trie pTbl is successfully walked through.
 *    MTRIE3L_ERR:     pTbl is NULL and/or f is NULL
 *
 *  output:
 *    None.
 *
 *
 * 7. Deleting All the Entries
 *
 * int mtrie3lDeleteAll (mtrie3l* pTbl,
 *                       void (*delEnt)(u32 index, void* dummy, void* pEnt));
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *    delEnt: function pointer called when index 'index' has an entry pointer.
 *            (*delEnt)() has the following parameters:
 *              - index: index at which pEnt is located.
 *              - dummy: not used.
 *              - pEnt:  pointer at index 'index'. pEnt needs to be freed.
 *
 *  return:
 *    MTRIE3L_SUCCESS: trie pTbl is successfully emptied.
 *    MTRIE3L_ERR:     pTbl is NULL and/or f is NULL
 *
 *  output:
 *    None.
 *
 *
 * 8. Finding the next entry to the specified index
 *
 * void* mtrie3lFindNext (mtrie3l* pTbl, u32* pIndex);
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *    pIndex: pointer to the index at which trie walk-through starts.
 *
 *  return:
 *    NULL if no more entries in the trie or pTbl is NULL.
 *    Valid pointer otherwise.
 *
 *  output:
 *    *pIndex: index at which the returned pointer is stored.
 *
 *
 * 9. Finding the previous entry to the specified index
 *
 * void* mtrie3lFindPrev (mtrie3l* pTbl, u32* pIndex);
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *    pIndex: pointer to the index at which trie walk-through starts.
 *
 *  return:
 *    NULL if no more entries in the trie or pTbl is NULL.
 *    Valid pointer otherwise.
 *
 *  output:
 *    *pIndex: index at which the returned pointer is stored.
 *
 *
 * 10. Maximum number of index in the table
 *
 * u32
 * mtrie3lgetMaxIndex (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The maximum index in trie pointed to by pTbl or
 *    maximum number of entries that pTbl can hold.
 *    This is an inline function.
 *
 *  output:
 *    None.
 *
 *
 * 11. Number of entries in the table
 *
 * u32
 * mtrie3lNumEntries (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The number of entries in trie pointed to by pTbl.
 *
 *  output:
 *    None.
 *
 *
 * 12. Number of all the level 1 nodes in the table
 *
 * u32
 * mtrie3lNumL1 (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The number of all the level 1 nodes in trie pointed to by pTbl.
 *
 *  output:
 *    None.
 *
 *
 * 13. Number of all the level 2 nodes in the table
 *
 * u32
 * mtrie3lNumL2 (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The number of all the level 2 nodes in trie pointed to by pTbl.
 *
 *  output:
 *    None.
 *
 *
 * 14. Size of the level 0 node
 *
 * u32
 * mtrie3lL0nodeSize (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The size of the level 0 node in bytes
 *
 *  output:
 *    None.
 *
 *
 * 15. Size of a level 1 node
 *
 * u32
 * mtrie3lL1nodeSize (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The size of a level 1 node in bytes
 *
 *  output:
 *    None.
 *
 *
 * 16. Size of a level 2 node
 *
 * u32
 * mtrie3lL2nodeSize (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    The size of a level 2 node in bytes
 *
 *  output:
 *    None.
 *
 *
 * 17. Total size of the currently allocated level 1 nodes in bytes
 *
 * u32
 * mtrie3lNbytesL1 (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    Total size of the currently allocated level 1 node in bytes
 *
 *  output:
 *    None.
 *
 *
 * 18. Total size of the currently allocated level 2 nodes in bytes
 *
 * u32
 * mtrie3lNbytesL2 (mtrie3l* pTbl)
 *
 *  input:
 *    pTbl:   pointer to a multibit trie.
 *
 *  return:
 *    Total size of the currently allocated level 2 nodes in bytes
 *
 *  output:
 *    None.
 *
 */
#endif /* __MTRIE3L_H__ */
