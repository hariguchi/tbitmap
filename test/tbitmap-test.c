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
 * tbitmap-test.c: make a command to test tbitmap,
 *                 a multibit trie bitmap library
 */

#include <assert.h>
#include "tbitmap.h"

/*
 * fib(n) = fib(n-1) + fib(n-2), f(0) = 0, f(1) = 1
 *
 * 0..21 0, 34..55 1, 89 2, 144 4, 233 7, 377 11, 610 19, 987 30
 * 1597 49, 2584 80, 4181 130, 6765 211
 */
static u32 Fib[] = {
            0,          1,         2,         3,         5, /*  0 -  4 */
            8,         13,        21,        34,        55, /*  5 -  9 */
           89,        144,       233,       377,       610, /* 10 - 14 */
          987,       1597,      2584,      4181,      6765, /* 15 - 19 */
        10946,      17711,     28657,     46368,     75025, /* 20 - 24 */
       121393,     196418,    317811,    514229,    832040, /* 25 - 29 */
      1346269,    2178309,   3524578,   5702887,   9227465, /* 30 - 34 */
     14930352,   24157817,  39088169,  63245986, 102334155, /* 35 - 39 */
    165580141,  267914296, 433494437,                       /* 40 - 42 */
};

static u32 UnsetBits[] = {
            4,         6,         7,         9,        11, /*  0 -  4 */
           12,        14,        15,        16,        17, /*  5 -  9 */
           18,        19,        20,        22,        23, /* 10 - 14 */
           24,        25,        26,        27,        28, /* 15 - 19 */
           29,        30,        31,        32,        33, /* 20 - 24 */
           35,        40,        50,        60,        63, /* 25 - 29 */
    100000000, 200000000, 300000000, 400000000, 500000000, /* 30 - 34 */
};


int
setResetTest (void)
{
    tBitMap   *p;
    tBitMapL2 *pL2;
    bool      isSet;
    int       i;
    int       num;
    int       rt;

    num = elementsOf(Fib);
    p   = tBitMapAlloc(Fib[num-1]);
    if (!p) {
        printf("%s: Error: can't alloc trie bitmap.\n", __FUNCTION__);
        assert(p);
    }

    /*
     * Set Fib[i] bits
     */
    for (i = 0; i < num; ++i) {
        tBitMapSet(p, Fib[i]);
        if (i == 8) {
            assert(Fib[i] == 34);
            assert(p->pTrie->num == 2);
            assert(p->pTrie->nL1 == 1);
            assert(p->pTrie->nL2 == 1);
            assert(p->pTrie->l0[0]->cnt == 1);
            pL2 = (tBitMapL2*)p->pTrie->l0[0]->l1[0];
            assert(pL2->cnt == 2);
        }
        if (i == 10) {
            assert(Fib[i] == 89);
            assert(p->pTrie->num == 3);
            assert(p->pTrie->nL1 == 1);
            assert(p->pTrie->nL2 == 1);
            assert(p->pTrie->l0[0]->cnt == 1);
            pL2 = (tBitMapL2*)p->pTrie->l0[0]->l1[0];
            assert(pL2->cnt == 3);
        }
        if (i == 19) {
            assert(Fib[i] == 6765);
            assert(p->pTrie->num == 12);
            assert(p->pTrie->nL1 == 1);
            assert(p->pTrie->nL2 == 1);
            assert(p->pTrie->l0[0]->cnt == 1);
            pL2 = (tBitMapL2*)p->pTrie->l0[0]->l1[0];
            assert(pL2->cnt == 12);
        }
        if (i == 20) {
            assert(Fib[i] == 10946);
            assert(p->pTrie->num == 13);
            assert(p->pTrie->nL1 == 1);
            assert(p->pTrie->nL2 == 2);
            assert(p->pTrie->l0[0]->cnt == 2);
            pL2 = (tBitMapL2*)p->pTrie->l0[0]->l1[1];
            assert(pL2->cnt == 1);
        }
    }
    assert(p->pTrie->num == 35);
    assert(p->pTrie->nL1 == 12);
    assert(p->pTrie->nL2 == 24);

    /*
     * Set bits[30:0] of bitmap at L0[100], L1[101], L2[102].
     */
    rt = tBitMapSetBlock(p, 210545856, 210545856 + 30);
    if (rt != TBITMAP_SUCCESS) {
        printf("%s: %d: Error (%d): tBitMapSetBlock()\n",
               __FUNCTION__, __LINE__, rt);
        assert(rt == TBITMAP_SUCCESS);
    }
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 25);
    assert(p->pTrie->num == 36);
    assert(p->pTrie->l0[100]->cnt == 1);
   /*
    * Set bit 31 of bitmap at L0[100], L1[101], L2[102].
    * Observe increment of pL2->nSetAll
    */
    rt = tBitMapSet(p, 210545856 + 31);
    if (rt != TBITMAP_SUCCESS) {
        printf("%s: %d: Error (%d): tBitMapSetBlock()\n",
               __FUNCTION__, __LINE__, rt);
        assert(rt == TBITMAP_SUCCESS);
    }
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 25);
    assert(p->pTrie->l0[100]->cnt == 1);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[100]->l1[101]);
    assert(pL2);
    assert(pL2->nSetAll == 1);
    assert(p->pTrie->num == 36);

    /*
     * Set bits[31:0] of bitmap at L0[100], L1[101], L2[0:254].
     */
    for (i = 0; i < 255; ++i) {
        rt = tBitMapSetBlock(p, 210542592 + (i << 5),
                                210542592 + (i << 5) + 31);
        assert(rt == TBITMAP_SUCCESS);
    }
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 25);
    assert(p->pTrie->l0[100]->cnt == 1);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[100]->l1[101]);
    assert(pL2);
    assert(pL2->nSetAll == 255);
    assert(p->pTrie->num == 290);
    /*
     * Set bits[31:0] of bitmap at L0[100], L1[101], L2[255].
     * Check if L2 node compression works correctly.
     */
    rt = tBitMapSetBlock(p, 210542592 + (i << 5),
                            210542592 + (i << 5) + 31);
    assert(p->pTrie->l0[100]->cnt == 1);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[100]->l1[101]);
    assert(pL2 == NULL);
    assert(getPtrTag(p->pTrie->l0[100]->l1[101]) == 1);
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 24);
    assert(p->pTrie->num == 291);

    /*
     * Reset bit 31 of bitmap at L0[100], L1[101], L2[255].
     * Check if uncompression works correctly.
     */
    rt = tBitMapReset(p, 210542592 + (i << 5) + 31);
    assert(p->pTrie->l0[100]->cnt == 1);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[100]->l1[101]);
    assert(pL2);
    assert(pL2->nSetAll == 255);
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 25);
    assert(pL2->bitmap[255] == 0x7fffffff);
    assert(p->pTrie->num == 291);

    /*
     * Reset bits[30:0] of bitmap at L0[100], L1[101], L2[255].
     * Check if uncompression works correctly.
     */
    rt = tBitMapResetBlock(p, 210542592 + (i << 5),
                           210542592 + (i << 5) + 30);
    assert(p->pTrie->l0[100]->cnt == 1);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[100]->l1[101]);
    assert(pL2);
    assert(pL2->nSetAll == 255);
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 25);
    assert(pL2->bitmap[255] == 0);
    assert(p->pTrie->num == 290);

    /*
     * Reset bits[31:0] of bitmaps at L0[100], L1[101], L2[254:1].
     */
    for (i = 254; i > 0; --i) {
        rt = tBitMapResetBlock(p, 210542592 + (i << 5),
                                210542592 + (i << 5) + 31);
        assert(rt == TBITMAP_SUCCESS);
    }
    assert(p->pTrie->l0[100]->cnt == 1);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[100]->l1[101]);
    assert(pL2);
    assert(pL2->nSetAll == 1);
    assert(p->pTrie->nL1 == 13);
    assert(p->pTrie->nL2 == 25);
    for (i = 1; i <= 255; ++i) {
        assert(pL2->bitmap[i] == 0);
    }
    assert(p->pTrie->num == 36);

    /*
     * Reset bits[31:0] of bitmap at L0[100], L1[101], L2[0].
     * Check if the L2 node is correctly removed and freed.
     */
    rt = tBitMapResetBlock(p, 210542592, 210542592 + 31);
    assert(rt == TBITMAP_SUCCESS);
    assert(p->pTrie->l0[100] == NULL);
    assert(p->pTrie->nL1 == 12);
    assert(p->pTrie->nL2 == 24);
    assert(p->pTrie->num == 35);

    /*
     * Check if bit UnsetBits[i] is set.
     * None of them should be set.
     */
    for (i = 0; i < elementsOf(UnsetBits); ++i) {
        isSet = tBitMapIsSet(p, UnsetBits[i]);
        if (isSet) {
            printf("%s: Error: bit %d is wrongly set.\n",
                   __FUNCTION__, UnsetBits[i]);
            assert(isSet);
        }
    }
    /*
     * Check if bit Fi[i] is set.
     * All of them should be set.
     */
    for (i = 0; i < num; ++i) {
        isSet = tBitMapIsSet(p, Fib[i]);
        if (!isSet) {
            printf("%s: Error: bit %d is wrongly unset.\n",
                   __FUNCTION__, Fib[i]);
            assert(!isSet);
        }
    }

    /*
     * Reset all the remaining bits.
     */
    rt = tBitMapResetBlock(p, Fib[0], Fib[num-1]);
    assert(rt == TBITMAP_SUCCESS);
    assert(p->pTrie->nL1 == 0);
    assert(p->pTrie->nL2 == 0);
    assert(p->pTrie->num == 0);

    /*
     * Set from: bit 30 of bitmap at L0[1], L1[255], L2[255]
     *     to:   bit  1 of bitmap at L0[4], L1[0],   L2[0]
     * # of bitmaps having at least 1 set bit is 131074 (1 + 2*256*256 + 1)
     */
    rt = tBitMapSetBlock(p, 4194302, 8388609);
    assert(rt == TBITMAP_SUCCESS);
    assert(p->pTrie->nL1 == 4);
    assert(p->pTrie->nL2 == 2);
    assert(p->pTrie->num == 131074);
    assert(p->pTrie->l0[1]->cnt == 1);
    assert(p->pTrie->l0[2]->cnt == 256);
    assert(p->pTrie->l0[3]->cnt == 256);
    assert(p->pTrie->l0[4]->cnt == 1);
    for (i = 0; i < 256; ++i) {
        assert(getPtrTag(p->pTrie->l0[2]->l1[i]) == 1);
        assert(getPtrTag(p->pTrie->l0[3]->l1[i]) == 1);
    }
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[1]->l1[255]);
    assert(pL2);
    assert(pL2->nSetAll == 0);
    assert(pL2->bitmap[255] == 0xc0000000);
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[4]->l1[0]);
    assert(pL2);
    assert(pL2->nSetAll == 0);
    assert(pL2->bitmap[0] == 3);

    /*
     * Reset from: bit 31 of bitmap at L0[2], L1[255], L2[255]
     *       to:   bit 30 of bitmap at L0[3], L1[1], L2[1]
     */
    rt = tBitMapResetBlock(p, 6291455, 6299710);
    assert(rt == TBITMAP_SUCCESS);
    assert(p->pTrie->nL1 == 4);
    assert(p->pTrie->nL2 == 4);
    assert(p->pTrie->num == 130817);
    assert(p->pTrie->l0[2]->cnt == 256);
    assert(p->pTrie->l0[3]->cnt == 255);
    assert(p->pTrie->l0[4]->cnt == 1);
    /*
     * Verifying L0[2]
     */
    for (i = 0; i < 255; ++i) {
        assert(getPtrTag(p->pTrie->l0[2]->l1[i]) == 1);
    }
    assert(getPtrTag(p->pTrie->l0[2]->l1[i]) == 0); /* L0[2], L1[255] */
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[2]->l1[i]);
    assert(pL2);
    assert(pL2->cnt == 256);
    assert(pL2->nSetAll == 255);
    for (i = 0; i < 255; ++i) {
        assert(pL2->bitmap[i] == ~0);
    }
    assert(pL2->bitmap[255] == 0x7fffffff);
    /*
     * Verifying L0[3]
     */
    assert(getPtrTag(p->pTrie->l0[3]->l1[0]) == 0);
    assert(getPtrTag(p->pTrie->l0[3]->l1[1]) == 0);
    for (i = 2; i < 256; ++i) {
        assert(getPtrTag(p->pTrie->l0[3]->l1[i]) == 1);
    }
    pL2 = getPtr(tBitMapL2, p->pTrie->l0[3]->l1[1]);
    assert(pL2);
    assert(pL2->cnt == 255);
    assert(pL2->nSetAll == 254);
    assert(pL2->bitmap[0] == 0);
    assert(pL2->bitmap[1] == 0x80000000);
    for (i = 2; i < 256; ++i) {
        assert(pL2->bitmap[i] == ~0);
    }

    rt = tBitMapFree(p);
    assert(rt == TBITMAP_SUCCESS);

    return rt;
}


int
main (int argc, char* argv[])
{
    int rt;

    rt = setResetTest();
    if (rt != TBITMAP_SUCCESS) {
        printf("Error: %d: singleSetResetTest()\n", rt);
    }
    exit(0);
    return 0;
}
    
