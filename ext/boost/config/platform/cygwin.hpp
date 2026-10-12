
#define BOOST_PLATFORM "Cygwin"
#define BOOST_HAS_DIRENT_H
#define BOOST_HAS_LOG1P
#define BOOST_HAS_EXPM1

#define BOOST_HAS_UNISTD_H
#include <unistd.h>
#if defined(_POSIX_THREADS) && (_POSIX_THREADS+0 >= 0) && !defined(BOOST_HAS_WINTHREADS)
#  define BOOST_HAS_PTHREADS
#  define BOOST_HAS_SCHED_YIELD
#  define BOOST_HAS_GETTIMEOFDAY
#  define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE

#else
#  if !defined(BOOST_HAS_WINTHREADS)
#     define BOOST_HAS_WINTHREADS
#  endif
#  define BOOST_HAS_FTIME
#endif

#include <sys/types.h>
#ifdef _STDINT_H
#define BOOST_HAS_STDINT_H
#endif
#if __GNUC__ > 5 && !defined(BOOST_HAS_STDINT_H)
#   define BOOST_HAS_STDINT_H
#endif

#include <cygwin/version.h>
#if (CYGWIN_VERSION_API_MAJOR == 0 && CYGWIN_VERSION_API_MINOR < 231)

#define BOOST_NO_FENV_H
#endif

#ifndef BOOST_NO_CXX14_HDR_SHARED_MUTEX
#include <pthread.h>
#if !(__XSI_VISIBLE >= 500 || __POSIX_VISIBLE >= 200112)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif
#endif

#include <boost/config/detail/posix_features.hpp>

#ifdef BOOST_HAS_NL_TYPES_H
#  undef BOOST_HAS_NL_TYPES_H
#endif
