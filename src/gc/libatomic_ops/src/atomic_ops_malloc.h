/*
 * Copyright (c) 2005 Hewlett-Packard Development Company, L.P.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Almost lock-free malloc implementation based on stack implementation. */
/* See README.malloc file for detailed usage rules.                      */

#ifndef MK_AO_ATOMIC_H
#define MK_AO_ATOMIC_H

#include <stdlib.h>     /* For size_t */

#include "atomic_ops_stack.h"

#ifdef MK_AO_STACK_IS_LOCK_FREE
# define MK_AO_MALLOC_IS_LOCK_FREE
#endif

void MK_AO_free(void *);

void * MK_AO_malloc(size_t);

/* Allow use of mmap to grow the heap.  No-op on some platforms.        */
void MK_AO_malloc_enable_mmap(void);

#endif /* !MK_AO_ATOMIC_H */
