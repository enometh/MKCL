/*
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999-2003 by Hewlett-Packard Company. All rights reserved.
 *
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 * Some of the machine specific code was borrowed from our GC distribution.
 */

#include "../all_aligned_atomic_load_store.h"

/* Real X86 implementations, appear                                     */
/* to enforce ordering between memory operations, EXCEPT that a later   */
/* read can pass earlier writes, presumably due to the visible          */
/* presence of store buffers.                                           */
/* We ignore the fact that the official specs                           */
/* seem to be much weaker (and arguably too weak to be usable).         */

#include "../ordered_except_wr.h"

#include "../test_and_set_t_is_char.h"

#include "../standard_ao_double_t.h"

MK_AO_INLINE void
MK_AO_nop_full(void)
{
  /* Note: "mfence" (SSE2) is supported on all x86_64/amd64 chips.      */
  __asm__ __volatile__ ("mfence" : : : "memory");
}
#define MK_AO_HAVE_nop_full

/* As far as we can tell, the lfence and sfence instructions are not    */
/* currently needed or useful for cached memory accesses.               */

MK_AO_INLINE MK_AO_t
MK_AO_fetch_and_add_full (volatile MK_AO_t *p, MK_AO_t incr)
{
  MK_AO_t result;

  __asm__ __volatile__ ("lock; xaddq %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr) /* , "m" (*p) */
                        : "memory");
  return result;
}
#define MK_AO_HAVE_fetch_and_add_full

MK_AO_INLINE unsigned char
MK_AO_char_fetch_and_add_full (volatile unsigned char *p, unsigned char incr)
{
  unsigned char result;

  __asm__ __volatile__ ("lock; xaddb %0, %1" :
                        "=q" (result), "=m" (*p) : "0" (incr) /* , "m" (*p) */
                        : "memory");
  return result;
}
#define MK_AO_HAVE_char_fetch_and_add_full

MK_AO_INLINE unsigned short
MK_AO_short_fetch_and_add_full (volatile unsigned short *p, unsigned short incr)
{
  unsigned short result;

  __asm__ __volatile__ ("lock; xaddw %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr) /* , "m" (*p) */
                        : "memory");
  return result;
}
#define MK_AO_HAVE_short_fetch_and_add_full

MK_AO_INLINE unsigned int
MK_AO_int_fetch_and_add_full (volatile unsigned int *p, unsigned int incr)
{
  unsigned int result;

  __asm__ __volatile__ ("lock; xaddl %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr) /* , "m" (*p) */
                        : "memory");
  return result;
}
#define MK_AO_HAVE_int_fetch_and_add_full

MK_AO_INLINE void
MK_AO_or_full (volatile MK_AO_t *p, MK_AO_t incr)
{
  __asm__ __volatile__ ("lock; orq %1, %0" :
                        "=m" (*p) : "r" (incr) /* , "m" (*p) */
                        : "memory");
}
#define MK_AO_HAVE_or_full

MK_AO_INLINE MK_AO_TS_VAL_t
MK_AO_test_and_set_full (volatile MK_AO_TS_t *addr)
{
  MK_AO_TS_t oldval;
  /* Note: the "xchg" instruction does not need a "lock" prefix */
  __asm__ __volatile__ ("xchg %b0, %1"
                        : "=q"(oldval), "=m"(*addr)
                        : "0"(0xff) /* , "m"(*addr) */
                        : "memory");
  return (MK_AO_TS_VAL_t)oldval;
}
#define MK_AO_HAVE_test_and_set_full

/* Returns nonzero if the comparison succeeded. */
MK_AO_INLINE int
MK_AO_compare_and_swap_full (volatile MK_AO_t *addr, MK_AO_t old, MK_AO_t new_val)
{
  char result;
  __asm__ __volatile__ ("lock; cmpxchgq %2, %0; setz %1"
                        : "=m"(*addr), "=a"(result)
                        : "r" (new_val), "a"(old) : "memory");
  return (int) result;
}
#define MK_AO_HAVE_compare_and_swap_full

#ifdef MK_AO_CMPXCHG16B_AVAILABLE
/* NEC LE-IT: older AMD Opterons are missing this instruction.
 * On these machines SIGILL will be thrown.
 * Define MK_AO_WEAK_DOUBLE_CAS_EMULATION to have an emulated
 * (lock based) version available */
/* HB: Changed this to not define either by default.  There are
 * enough machines and tool chains around on which cmpxchg16b
 * doesn't work.  And the emulation is unsafe by our usual rules.
 * Hoewever both are clearly useful in certain cases.
 */
MK_AO_INLINE int
MK_AO_compare_double_and_swap_double_full (volatile MK_AO_double_t *addr,
                                        MK_AO_t old_val1, MK_AO_t old_val2,
                                        MK_AO_t new_val1, MK_AO_t new_val2)
{
  char result;
  __asm__ __volatile__ ("lock; cmpxchg16b %0; setz %1"
                        : "=m"(*addr), "=a"(result)
                        : "m"(*addr), "d" (old_val2), "a" (old_val1),
                          "c" (new_val2), "b" (new_val1) : "memory");
  return (int) result;
}
#define MK_AO_HAVE_compare_double_and_swap_double_full
#else
/* this one provides spinlock based emulation of CAS implemented in     */
/* atomic_ops.c.  We probably do not want to do this here, since it is  */
/* not atomic with respect to other kinds of updates of *addr.  On the  */
/* other hand, this may be a useful facility on occasion.               */
#ifdef MK_AO_WEAK_DOUBLE_CAS_EMULATION
int MK_AO_compare_double_and_swap_double_emulation(volatile MK_AO_double_t *addr,
                                                MK_AO_t old_val1, MK_AO_t old_val2,
                                                MK_AO_t new_val1, MK_AO_t new_val2);

MK_AO_INLINE int
MK_AO_compare_double_and_swap_double_full(volatile MK_AO_double_t *addr,
                                       MK_AO_t old_val1, MK_AO_t old_val2,
                                       MK_AO_t new_val1, MK_AO_t new_val2)
{
        return MK_AO_compare_double_and_swap_double_emulation(addr,
                                                           old_val1, old_val2,
                                                           new_val1, new_val2);
}
#define MK_AO_HAVE_compare_double_and_swap_double_full
#endif /* MK_AO_WEAK_DOUBLE_CAS_EMULATION */
#endif /* MK_AO_CMPXCHG16B_AVAILABLE */
