
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__DragonFly__)
#error "This platform is not BSD"
#endif

#ifdef __FreeBSD__
#define BOOST_PLATFORM "FreeBSD " BOOST_STRINGIZE(__FreeBSD__)
#elif defined(__NetBSD__)
#define BOOST_PLATFORM "NetBSD " BOOST_STRINGIZE(__NetBSD__)
#elif defined(__OpenBSD__)
#define BOOST_PLATFORM "OpenBSD " BOOST_STRINGIZE(__OpenBSD__)
#elif defined(__DragonFly__)
#define BOOST_PLATFORM "DragonFly " BOOST_STRINGIZE(__DragonFly__)
#endif

#if (defined(__FreeBSD__) && (__FreeBSD__ >= 3)) \
   || defined(__OpenBSD__) || defined(__DragonFly__)
#  define BOOST_HAS_NL_TYPES_H
#endif

#if (defined(__FreeBSD__) && (__FreeBSD__ <= 3))\
   || defined(__OpenBSD__) || defined(__DragonFly__)
#  define BOOST_HAS_PTHREADS
#endif

#if defined(__NetBSD__)
#define __NetBSD_GCC__ (__GNUC__         * 1000000 \
                       + __GNUC_MINOR__ *    1000 \
                       + __GNUC_PATCHLEVEL__)

#define _GLIBCXX_HAVE_SWPRINTF 1
#endif

#if !((defined(__FreeBSD__) && (__FreeBSD__ >= 5)) \
      || (defined(__NetBSD_GCC__) && (__NetBSD_GCC__ >= 2095003)) \
      || defined(__OpenBSD__) || defined(__DragonFly__))
#  define BOOST_NO_CWCHAR
#endif

#if !defined(__OpenBSD__) || defined(__DragonFly__)
#  define BOOST_NO_CTYPE_FUNCTIONS
#endif

#define BOOST_HAS_SCHED_YIELD
#define BOOST_HAS_NANOSLEEP
#define BOOST_HAS_GETTIMEOFDAY
#define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#define BOOST_HAS_SIGACTION
#define BOOST_HAS_CLOCK_GETTIME

#define BOOST_HAS_UNISTD_H
#include <boost/config/detail/posix_features.hpp>
