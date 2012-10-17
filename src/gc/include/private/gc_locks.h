/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999 by Hewlett-Packard Company. All rights reserved.
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
 */

#ifndef MK_GC_LOCKS_H
#define MK_GC_LOCKS_H

/*
 * Mutual exclusion between allocator/collector routines.
 * Needed if there is more than one allocator thread.
 * DCL_LOCK_STATE declares any local variables needed by LOCK and UNLOCK.
 *
 * Note that I_HOLD_LOCK and I_DONT_HOLD_LOCK are used only positively
 * in assertions, and may return TRUE in the "don't know" case.
 */
# ifdef THREADS

#  if defined(MK_GC_PTHREADS) && !defined(MK_GC_WIN32_THREADS)
#    include "atomic_ops.h"
#  endif

   MK_GC_API void MK_GC_CALL MK_GC_noop1(word);
#  ifdef PCR
#    include <base/PCR_Base.h>
#    include <th/PCR_Th.h>
     MK_GC_EXTERN PCR_Th_ML MK_GC_allocate_ml;
#    define DCL_LOCK_STATE \
         PCR_ERes MK_GC_fastLockRes; PCR_sigset_t MK_GC_old_sig_mask
#    define UNCOND_LOCK() PCR_Th_ML_Acquire(&MK_GC_allocate_ml)
#    define UNCOND_UNLOCK() PCR_Th_ML_Release(&MK_GC_allocate_ml)
#  endif

#  if !defined(MK_AO_HAVE_test_and_set_acquire) && defined(MK_GC_PTHREADS)
#    define USE_PTHREAD_LOCKS
#  endif

#  if defined(MK_GC_WIN32_THREADS) && defined(MK_GC_PTHREADS)
#    define USE_PTHREAD_LOCKS
#  endif

#  if defined(MK_GC_RTEMS_PTHREADS)
#    define USE_PTHREAD_LOCKS
#  endif

#  if defined(MK_GC_WIN32_THREADS) && !defined(USE_PTHREAD_LOCKS)
#    ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN 1
#    endif
#    define NOSERVICE
#    include <windows.h>
#    define NO_THREAD (DWORD)(-1)
     MK_GC_EXTERN DWORD MK_GC_lock_holder;
     MK_GC_EXTERN CRITICAL_SECTION MK_GC_allocate_ml;
#    ifdef MK_GC_ASSERTIONS
#        define UNCOND_LOCK() \
                { EnterCriticalSection(&MK_GC_allocate_ml); \
                  SET_LOCK_HOLDER(); }
#        define UNCOND_UNLOCK() \
                { MK_GC_ASSERT(I_HOLD_LOCK()); UNSET_LOCK_HOLDER(); \
                  LeaveCriticalSection(&MK_GC_allocate_ml); }
#    else
#      define UNCOND_LOCK() EnterCriticalSection(&MK_GC_allocate_ml)
#      define UNCOND_UNLOCK() LeaveCriticalSection(&MK_GC_allocate_ml)
#    endif /* !MK_GC_ASSERTIONS */
#    define SET_LOCK_HOLDER() MK_GC_lock_holder = GetCurrentThreadId()
#    define UNSET_LOCK_HOLDER() MK_GC_lock_holder = NO_THREAD
#    define I_HOLD_LOCK() (!MK_GC_need_to_lock \
                           || MK_GC_lock_holder == GetCurrentThreadId())
#    define I_DONT_HOLD_LOCK() (!MK_GC_need_to_lock \
                           || MK_GC_lock_holder != GetCurrentThreadId())
#  elif defined(SN_TARGET_PS3)
#    include <pthread.h>
     MK_GC_EXTERN pthread_mutex_t MK_GC_allocate_ml;
#    define LOCK() pthread_mutex_lock(&MK_GC_allocate_ml)
#    define UNLOCK() pthread_mutex_unlock(&MK_GC_allocate_ml)
#  elif defined(MK_GC_PTHREADS)
#    include <pthread.h>

     /* Posix allows pthread_t to be a struct, though it rarely is.     */
     /* Unfortunately, we need to use a pthread_t to index a data       */
     /* structure.  It also helps if comparisons don't involve a        */
     /* function call.  Hence we introduce platform-dependent macros    */
     /* to compare pthread_t ids and to map them to integers.           */
     /* the mapping to integers does not need to result in different    */
     /* integers for each thread, though that should be true as much    */
     /* as possible.                                                    */
     /* Refine to exclude platforms on which pthread_t is struct */
#    if !defined(MK_GC_WIN32_PTHREADS)
#      define NUMERIC_THREAD_ID(id) ((unsigned long)(id))
#      define THREAD_EQUAL(id1, id2) ((id1) == (id2))
#      define NUMERIC_THREAD_ID_UNIQUE
#    else
#      define NUMERIC_THREAD_ID(id) ((unsigned long)(id.p))
       /* Using documented internal details of win32-pthread library.   */
       /* Faster than pthread_equal(). Should not change with           */
       /* future versions of win32-pthread library.                     */
#      define THREAD_EQUAL(id1, id2) ((id1.p == id2.p) && (id1.x == id2.x))
#      undef NUMERIC_THREAD_ID_UNIQUE
       /* Generic definitions based on pthread_equal() always work but  */
       /* will result in poor performance (as NUMERIC_THREAD_ID is      */
       /* defined to just a constant) and weak assertion checking.      */
#    endif
#    define NO_THREAD ((unsigned long)(-1l))
                /* != NUMERIC_THREAD_ID(pthread_self()) for any thread */

#    if !defined(THREAD_LOCAL_ALLOC) && !defined(USE_PTHREAD_LOCKS)
      /* In the THREAD_LOCAL_ALLOC case, the allocation lock tends to   */
      /* be held for long periods, if it is held at all.  Thus spinning */
      /* and sleeping for fixed periods are likely to result in         */
      /* significant wasted time.  We thus rely mostly on queued locks. */
#     define USE_SPIN_LOCK
      MK_GC_EXTERN volatile MK_AO_TS_t MK_GC_allocate_lock;
      MK_GC_INNER void MK_GC_lock(void);
        /* Allocation lock holder.  Only set if acquired by client through */
        /* MK_GC_call_with_alloc_lock.                                        */
#     ifdef MK_GC_ASSERTIONS
#        define UNCOND_LOCK() \
              { if (MK_AO_test_and_set_acquire(&MK_GC_allocate_lock) == MK_AO_TS_SET) \
                  MK_GC_lock(); \
                SET_LOCK_HOLDER(); }
#        define UNCOND_UNLOCK() \
              { MK_GC_ASSERT(I_HOLD_LOCK()); UNSET_LOCK_HOLDER(); \
                MK_AO_CLEAR(&MK_GC_allocate_lock); }
#     else
#        define UNCOND_LOCK() \
              { if (MK_AO_test_and_set_acquire(&MK_GC_allocate_lock) == MK_AO_TS_SET) \
                  MK_GC_lock(); }
#        define UNCOND_UNLOCK() MK_AO_CLEAR(&MK_GC_allocate_lock)
#     endif /* !MK_GC_ASSERTIONS */
#    else /* THREAD_LOCAL_ALLOC  || USE_PTHREAD_LOCKS */
#      ifndef USE_PTHREAD_LOCKS
#        define USE_PTHREAD_LOCKS
#      endif
#    endif /* THREAD_LOCAL_ALLOC || USE_PTHREAD_LOCK */
#    ifdef USE_PTHREAD_LOCKS
#      include <pthread.h>
       MK_GC_EXTERN pthread_mutex_t MK_GC_allocate_ml;
#      ifdef MK_GC_ASSERTIONS
#        define UNCOND_LOCK() { MK_GC_lock(); SET_LOCK_HOLDER(); }
#        define UNCOND_UNLOCK() \
                { MK_GC_ASSERT(I_HOLD_LOCK()); UNSET_LOCK_HOLDER(); \
                  pthread_mutex_unlock(&MK_GC_allocate_ml); }
#      else /* !MK_GC_ASSERTIONS */
#        if defined(NO_PTHREAD_TRYLOCK)
#          define UNCOND_LOCK() MK_GC_lock()
#        else /* !defined(NO_PTHREAD_TRYLOCK) */
#        define UNCOND_LOCK() \
           { if (0 != pthread_mutex_trylock(&MK_GC_allocate_ml)) \
               MK_GC_lock(); }
#        endif
#        define UNCOND_UNLOCK() pthread_mutex_unlock(&MK_GC_allocate_ml)
#      endif /* !MK_GC_ASSERTIONS */
#    endif /* USE_PTHREAD_LOCKS */
#    define SET_LOCK_HOLDER() \
                MK_GC_lock_holder = NUMERIC_THREAD_ID(pthread_self())
#    define UNSET_LOCK_HOLDER() MK_GC_lock_holder = NO_THREAD
#    define I_HOLD_LOCK() \
                (!MK_GC_need_to_lock || \
                 MK_GC_lock_holder == NUMERIC_THREAD_ID(pthread_self()))
#    ifndef NUMERIC_THREAD_ID_UNIQUE
#      define I_DONT_HOLD_LOCK() 1  /* Conservatively say yes */
#    else
#      define I_DONT_HOLD_LOCK() \
                (!MK_GC_need_to_lock \
                 || MK_GC_lock_holder != NUMERIC_THREAD_ID(pthread_self()))
#    endif
     MK_GC_EXTERN volatile MK_GC_bool MK_GC_collecting;
#    define ENTER_GC() MK_GC_collecting = 1;
#    define EXIT_GC() MK_GC_collecting = 0;
     MK_GC_INNER void MK_GC_lock(void);
     MK_GC_EXTERN unsigned long MK_GC_lock_holder;
#    if defined(MK_GC_ASSERTIONS) && defined(PARALLEL_MARK)
       MK_GC_EXTERN unsigned long MK_GC_mark_lock_holder;
#    endif
#  endif /* MK_GC_PTHREADS with linux_threads.c implementation */
   MK_GC_EXTERN MK_GC_bool MK_GC_need_to_lock;

# else /* !THREADS */
#   define LOCK()
#   define UNLOCK()
#   define SET_LOCK_HOLDER()
#   define UNSET_LOCK_HOLDER()
#   define I_HOLD_LOCK() TRUE
#   define I_DONT_HOLD_LOCK() TRUE
                /* Used only in positive assertions or to test whether  */
                /* we still need to acquire the lock.  TRUE works in    */
                /* either case.                                         */
# endif /* !THREADS */

#if defined(UNCOND_LOCK) && !defined(LOCK)
                /* At least two thread running; need to lock.   */
#    define LOCK() { if (MK_GC_need_to_lock) UNCOND_LOCK(); }
#    define UNLOCK() { if (MK_GC_need_to_lock) UNCOND_UNLOCK(); }
#endif

# ifndef ENTER_GC
#   define ENTER_GC()
#   define EXIT_GC()
# endif

# ifndef DCL_LOCK_STATE
#   define DCL_LOCK_STATE
# endif

#endif /* MK_GC_LOCKS_H */
