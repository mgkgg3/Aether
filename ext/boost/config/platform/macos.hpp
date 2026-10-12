
#define BOOST_PLATFORM "Mac OS"

#if __MACH__ && !defined(_MSL_USING_MSL_C)

#  ifndef BOOST_HAS_UNISTD_H
#    define BOOST_HAS_UNISTD_H
#  endif

#  include <boost/config/detail/posix_features.hpp>
#  ifndef BOOST_HAS_STDINT_H
#     define BOOST_HAS_STDINT_H
#  endif

#  define BOOST_HAS_SCHED_YIELD
#  define BOOST_HAS_GETTIMEOFDAY
#  define BOOST_HAS_SIGACTION

#  if (__GNUC__ < 3) && !defined( __APPLE_CC__)

#    define BOOST_NO_STDC_NAMESPACE
#  endif

#  if (__GNUC__ >= 4)

#    define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#    define BOOST_HAS_NANOSLEEP

#  endif

#else

#  if ( defined(TARGET_API_MAC_CARBON) && TARGET_API_MAC_CARBON ) || ( defined(TARGET_CARBON) && TARGET_CARBON )

#  if !defined(BOOST_HAS_PTHREADS)

#  elif ( __dest_os == __mac_os_x )

#  define BOOST_HAS_GETTIMEOFDAY
#  endif

#ifdef BOOST_HAS_PTHREADS
#  define BOOST_HAS_THREADS
#endif

#    define BOOST_BIND_ENABLE_PASCAL

#  endif

#endif
