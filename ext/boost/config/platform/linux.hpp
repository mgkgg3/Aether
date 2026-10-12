
#define BOOST_PLATFORM "linux"

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#if defined(__GLIBC__) && ((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1)))

#  if defined(__GNUC__) || ((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 5)))
#    define BOOST_HAS_STDINT_H
#  endif
#endif

#if defined(__LIBCOMO__)

#  if __LIBCOMO_VERSION__ <= 20
#    define BOOST_NO_STDC_NAMESPACE
#  endif

#  if __LIBCOMO_VERSION__ <= 21
#    define BOOST_NO_SWPRINTF
#  endif

#endif

#if defined(__GLIBC__) && (__GLIBC__ >= 2)
#  define BOOST_HAS_GETTIMEOFDAY
#endif

#ifdef __USE_POSIX199309
#  define BOOST_HAS_NANOSLEEP
#endif

#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)

#  if !__GLIBC_PREREQ(2,2) || (!defined(__USE_ISOC99) && !defined(__USE_UNIX98))
#    define BOOST_NO_SWPRINTF
#  endif
#else
#  define BOOST_NO_SWPRINTF
#endif

#define BOOST_HAS_UNISTD_H
#include <boost/config/detail/posix_features.hpp>
#if defined(__USE_GNU) && !defined(__ANDROID__) && !defined(ANDROID)
#define BOOST_HAS_PTHREAD_YIELD
#endif

#ifndef __GNUC__

#  ifndef __extension__
#     define __extension__
#  endif
#  ifndef __const__
#     define __const__ const
#  endif
#  ifndef __volatile__
#     define __volatile__ volatile
#  endif
#  ifndef __signed__
#     define __signed__ signed
#  endif
#  ifndef __typeof__
#     define __typeof__ typeof
#  endif
#  ifndef __inline__
#     define __inline__ inline
#  endif
#endif
