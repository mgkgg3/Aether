
#  if defined(BOOST_HAS_UNISTD_H)
#     include <unistd.h>

#     if defined(_XOPEN_VERSION) && (_XOPEN_VERSION >= 3)
#        define BOOST_HAS_NL_TYPES_H
#     endif

#     if defined(_POSIX_VERSION) && (_POSIX_VERSION >= 200100)
#        define BOOST_HAS_STDINT_H
#     endif

#     if defined(_POSIX_VERSION) && (_POSIX_VERSION >= 199009L)
#        define BOOST_HAS_DIRENT_H
#     endif

#     if defined(_POSIX_VERSION) && (_POSIX_VERSION >= 199506L)
#        define BOOST_HAS_SIGACTION
#     endif

#     if defined(_POSIX_THREADS) && (_POSIX_THREADS+0 >= 0) && !defined(BOOST_HAS_WINTHREADS) && !defined(BOOST_HAS_MPTASKS)
#        define BOOST_HAS_PTHREADS
#     endif

#     if (defined(_POSIX_TIMERS) && (_POSIX_TIMERS+0 >= 0)) \
             || (defined(_XOPEN_REALTIME) && (_XOPEN_REALTIME+0 >= 0))
#        define BOOST_HAS_NANOSLEEP
#     endif

#     if (defined(_POSIX_TIMERS) && (_POSIX_TIMERS+0 >= 0))
#        define BOOST_HAS_CLOCK_GETTIME
#     endif

#     if defined(_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING+0 > 0)\
            || (defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && (_POSIX_THREAD_PRIORITY_SCHEDULING+0 > 0))\
            || (defined(_XOPEN_REALTIME) && (_XOPEN_REALTIME+0 >= 0))
#        define BOOST_HAS_SCHED_YIELD
#     endif

#     if defined(_XOPEN_VERSION) && (_XOPEN_VERSION+0 >= 500)
#        define BOOST_HAS_GETTIMEOFDAY
#        if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE+0 >= 500)
#           define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#        endif
#        ifndef BOOST_HAS_LOG1P
#           define BOOST_HAS_LOG1P
#        endif
#        ifndef BOOST_HAS_EXPM1
#           define BOOST_HAS_EXPM1
#        endif
#     endif

#  endif
