
#define BOOST_PLATFORM "Sun Solaris"

#define BOOST_HAS_GETTIMEOFDAY

#define BOOST_HAS_UNISTD_H
#include <boost/config/detail/posix_features.hpp>

#if defined(__GNUC__) && defined(_POSIX_THREADS) && !defined(_PTHREADS)
# undef BOOST_HAS_PTHREADS
#endif

#define BOOST_HAS_STDINT_H
#define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#define BOOST_HAS_LOG1P
#define BOOST_HAS_EXPM1
